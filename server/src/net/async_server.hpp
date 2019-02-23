#pragma once

// project
#include "net/server_states.hpp"
#include "testing/testing.hpp"

// third-party
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

// standard
#include <mutex>

namespace net {

/**
 * @brief
 *
 *     When registering RPC functions with the async interface, RPC calls of the form:
 *
 *     ```proto
 *     package mypkg;
 *
 *     service MyService {
 *         rpc MyRpc (MyRequest) return (MyResponse);
 *     }
 *     ```
 *
 *     become `mypkg::MyService::AsyncService::RequestMyRpc` when accessing
 *     the functions in C++ land.
 *
 * @tparam Service - The gRPC service to use for this server (e.g. mypkg::MyService)
 */
template <typename Service>
class AsyncServer {
public:
    explicit AsyncServer(unsigned port);

    /**
     * @brief This specifies what will happen when a unary rpc call is triggered by the client.
     *
     * .............In 'echo.proto'.............
     *
     * package echo;
     *
     * service Echo {
     *     rpc TestEcho (EchoRequest) returns (EchoResponse);
     * }
     * .........................................
     *
     * ..............In 'echo.cpp'..............
     *
     * AsyncServer<echo::Echo> server("0.0.0.0:50051");
     *
     * // The callback that will be executed when a client calls 'TestEcho'
     * auto test_echo_callback = [] (const echo::EchoRequest* request, echo::EchoResponse* response) {
     *                               *response = request;
     *                                return grpc::Status::OK;
     *                           };
     *
     * server.register_unary_rpc(&echo::Echo::AsyncService::RequestTestEcho, test_echo_callback);
     * .........................................
     *
     * @param rpc_function - The unary RPC function
     * @param callback - What to do when this RPC is triggered <grpc::Status(const Request&, Response*)>
     */
    template <typename RpcFunction, typename ConnectCallback, typename DisconnectCallback = detail::EmptyDisconnect>
    void register_rpc(RpcFunction rpc_function,
                      ConnectCallback&& connect_callback,
                      DisconnectCallback&& disconnect_callback = {});

    void run();

    void shutdown();

private:
    using AsyncService = typename Service::AsyncService;

    std::unique_ptr<AsyncService> service_;
    std::unique_ptr<grpc::ServerCompletionQueue> server_queue_;
    std::unique_ptr<grpc::Server> server_;

    detail::Tagger tagger_;
    std::unordered_map<void*, std::unique_ptr<detail::RpcCallHandle<AsyncService>>> rpc_calls_;
    std::unordered_map<void*, std::unique_ptr<detail::Connection>> active_connections_;
    std::unordered_map<void*, void*> connections_to_rpc_calls_;

    std::mutex update_lock_;
};

template <typename Service>
AsyncServer<Service>::AsyncServer(unsigned port) : service_(std::make_unique<AsyncService>()) {
    std::string host_address = "0.0.0.0:" + std::to_string(port);

    std::cout << "Server running at " << host_address << std::endl;

    grpc::ServerBuilder builder;
    builder.RegisterService(service_.get());
    builder.AddListeningPort(host_address, grpc::InsecureServerCredentials());
    server_queue_ = builder.AddCompletionQueue();

    // Prevent multiple servers from running on the same port
    builder.AddChannelArgument("grpc.so_reuseport", 0);
    server_ = builder.BuildAndStart();

    if (!server_) {
        throw std::runtime_error("Failed to build server (might have one running on the same port).");
    }
}

template <typename Service>
template <typename RpcFunction, typename ConnectCallback, typename DisconnectCallback>
void AsyncServer<Service>::register_rpc(RpcFunction rpc_function,
                                        ConnectCallback&& connect_callback,
                                        DisconnectCallback&& disconnect_callback) {

    auto rpc_handle = detail::make_rpc_call_handle<AsyncService>(rpc_function,
                                                                 std::forward<ConnectCallback>(connect_callback),
                                                                 std::forward<DisconnectCallback>(disconnect_callback));

    rpc_handle->queue_next_client_connection(service_.get(), server_queue_.get(), &tagger_);

    void* tag_id = rpc_handle.get();
    rpc_calls_.emplace(tag_id, std::move(rpc_handle));
}

template <typename Service>
void AsyncServer<Service>::run() {
    void* tag_id;
    bool call_ok;

    while (server_queue_->Next(&tag_id, &call_ok)) {
        std::lock_guard<std::mutex> lock(update_lock_);

        detail::Tag tag{};
        unsigned tag_count{};

        std::tie(tag, tag_count) = tagger_.get_tag(tag_id);

        switch (tag.label) {

        case detail::TagLabel::rpc_call_requested_by_client:

            if (call_ok) {
                auto rpc_call = static_cast<detail::RpcCallHandle<AsyncService>*>(tag.data);

                auto active_connection = rpc_call->extract_active_connection();

                // Process the new connection if it hasn't already added itself to the queue (all connections
                // will have at least one tag on the queue to notify us when the connection is broken).
                if (tagger_.count(active_connection.get()) == 1u) {
                    active_connection->add_next_tag_to_queue();
                }
                void* connection_id = active_connection.get();
                active_connections_.emplace(connection_id, std::move(active_connection));
                connections_to_rpc_calls_.emplace(connection_id, rpc_call);

                rpc_call->queue_next_client_connection(service_.get(), server_queue_.get(), &tagger_);

            } else if (tag_count == 0) {
                rpc_calls_.erase(tag_id);
            }
            continue;

        case detail::TagLabel::processing:
            if (call_ok) {
                auto connection = static_cast<detail::Connection*>(tag.data);
                connection->add_next_tag_to_queue();
            }
            break;

        case detail::TagLabel::rpc_finished: {
            void* rpc_id = connections_to_rpc_calls_.at(tag.data);
            rpc_calls_.at(rpc_id)->disconnect(tag.data);
        } break;

        } // end switch

        if (tag_count == 0) {
            // No more tags with this active_connection are left in the queue so we can delete the data
            connections_to_rpc_calls_.erase(tag.data);
            active_connections_.erase(tag.data);
        }
    }
}

template <typename Server>
void AsyncServer<Server>::shutdown() {
    std::lock_guard<std::mutex> lock(update_lock_);
    for (auto& connection : active_connections_) {
        connection.second->cancel();
    }
    server_->Shutdown();
    server_queue_->Shutdown();
}

} // namespace net

#ifdef DOCTEST_LIBRARY_INCLUDED
#include <testing/test_client.hpp>
#include <thread>

template class net::AsyncServer<testing::proto::Echo>;

namespace tp = testing::proto;
using TestService = tp::Echo::AsyncService;

TEST_CASE("[net] test server can be stopped immediately with no RPCs") {
    unsigned port = 9090u;
    net::AsyncServer<testing::proto::Echo> server(/*port=*/port);

    std::thread run_thread([&server] { server.run(); });

    server.shutdown();
    run_thread.join();
}

TEST_CASE("[net] test single unary rpc call") {
    unsigned port = 9090u;

    net::AsyncServer<testing::proto::Echo> server(/*port=*/port);

    server.register_rpc(&TestService::RequestUnaryEchoTest, testing::TestService{});

    std::thread run_thread([&server] { server.run(); });

    std::string server_address = "0.0.0.0:" + std::to_string(port);
    testing::TestClient client(server_address);

    const char* test_message = "test message";

    grpc::ClientContext context;
    tp::EchoRequest request{};
    request.set_message(test_message);
    tp::EchoResponse response{};

    grpc::Status status = client.stub->UnaryEchoTest(&context, request, &response);

    REQUIRE(status.ok());
    CHECK(response.message() == test_message);

    server.shutdown();
    run_thread.join();
}

TEST_CASE("[net] test single streaming rpc call") {
    unsigned port = 9090u;

    net::AsyncServer<testing::proto::Echo> server(/*port=*/port);

    server.register_rpc(&TestService::RequestServerStreamEchoTest, testing::TestService{});

    std::thread run_thread([&server] { server.run(); });

    std::string server_address = "0.0.0.0:" + std::to_string(port);
    testing::TestClient client(server_address);

    const char* test_message = "test message";
    int expected_responses = 12;

    grpc::ClientContext context;
    tp::EchoRequest request{};
    request.set_message(test_message);
    request.set_expected_responses(expected_responses);

    std::unique_ptr<grpc::ClientReader<tp::EchoResponse>> response_reader;

    response_reader = client.stub->ServerStreamEchoTest(&context, request);
    REQUIRE(response_reader);

    int i = 0;
    for (; i < expected_responses + 1; ++i) {
        tp::EchoResponse response{};

        if (!response_reader->Read(&response)) {
            break;
        }
        CHECK(response.message() == test_message);
        CHECK(response.response_number() == i);
    }
    CHECK(i == expected_responses);

    grpc::Status status = response_reader->Finish();
    CHECK(status.ok());

    server.shutdown();
    run_thread.join();
}

TEST_CASE("[net] test continuous streaming rpc call returns correct pointer on disconnect") {
    unsigned port = 9090u;

    net::AsyncServer<testing::proto::Echo> server(/*port=*/port);

    void* connect_ptr = nullptr;
    void* disconnect_ptr = nullptr;

    server.register_rpc(&TestService::RequestServerStreamEchoTest,
                        [&connect_ptr](const testing::proto::EchoRequest&,
                                       net::ServerToClientStream<testing::proto::EchoResponse>* stream) {
                            connect_ptr = stream;
                            // Write an empty response so the client can wait on the read to sync usage
                            stream->write(testing::proto::EchoResponse{});
                        },
                        [&disconnect_ptr](void* stream) { disconnect_ptr = stream; });

    std::thread run_thread([&server] { server.run(); });

    std::string server_address = "0.0.0.0:" + std::to_string(port);
    testing::TestClient client(server_address);

    const char* test_message = "test message";
    int expected_responses = 12;

    grpc::ClientContext context;
    tp::EchoRequest request{};
    request.set_message(test_message);
    request.set_expected_responses(expected_responses);

    std::unique_ptr<grpc::ClientReader<tp::EchoResponse>> response_reader;

    response_reader = client.stub->ServerStreamEchoTest(&context, request);
    REQUIRE(response_reader);

    testing::proto::EchoResponse response;
    bool read_status = response_reader->Read(&response); // Wait for server to set connect_ptr

    CHECK(read_status);
    CHECK(connect_ptr != nullptr);

    server.shutdown();
    run_thread.join();

    // Server finished so the stream has been cancelled and should return a false status
    read_status = response_reader->Read(&response);

    CHECK_FALSE(read_status);
    CHECK(disconnect_ptr != nullptr);
    CHECK(disconnect_ptr == connect_ptr);

    // Status should not be ok since the stream was cancelled by the server
    grpc::Status status = response_reader->Finish();
    CHECK_FALSE(status.ok());
}
#endif

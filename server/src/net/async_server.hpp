#pragma once

// project
#include "net/server_states.hpp"

// third-party
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

// standard
#include <unordered_set>
#include <variant>

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
    template <typename RpcFunction, typename ConnectCallback, typename DisconnectCallback>
    void register_rpc(RpcFunction rpc_function,
                      ConnectCallback&& connect_callback,
                      DisconnectCallback&& disconnect_callback = [] {});

    void run();

private:
    using AsyncService = typename Service::AsyncService;

    std::unique_ptr<AsyncService> service_;
    std::unique_ptr<grpc::ServerCompletionQueue> server_queue_;
    std::unique_ptr<grpc::Server> server_;

    detail::Tagger tagger_;
    std::unordered_map<void*, std::unique_ptr<detail::RpcCall<AsyncService>>> rpc_calls_;
    std::unordered_map<void*, std::unique_ptr<detail::Connection>> active_connections_;
    std::unordered_map<void*, void*> connections_to_rpc_calls_;
};

template <typename Service>
AsyncServer<Service>::AsyncServer(unsigned port) : service_(std::make_unique<AsyncService>()) {
    std::string host_address = "0.0.0.0:" + std::to_string(port);

    std::cout << "Server running at " << host_address << std::endl;

    grpc::ServerBuilder builder;
    builder.RegisterService(service_.get());
    builder.AddListeningPort(host_address, grpc::InsecureServerCredentials());
    server_queue_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
}

template <typename Service>
template <typename RpcFunction, typename Callback>
void AsyncServer<Service>::register_rpc(RpcFunction rpc_function, Callback&& callback) {

    auto rpc_handle = detail::make_rpc_call_handle<AsyncService>(rpc_function, std::forward<Callback>(callback));

    rpc_handle->queue_next_client_connection(service_.get(), server_queue_.get(), &tagger_);

    void* tag_id = rpc_handle.get();
    rpc_calls_.emplace(tag_id, std::move(rpc_handle));
}

template <typename Service>
void AsyncServer<Service>::run() {
    void* tag_id;
    bool call_ok;

    while (server_queue_->Next(&tag_id, &call_ok)) {
        detail::Tag tag{};
        unsigned tag_count;

        std::tie(tag, tag_count) = tagger_.get_tag(tag_id);

        switch (tag.label) {

        case detail::TagLabel::rpc_call_requested_by_client: {
            std::cout << "rpc_call_requested_by_client" << std::endl;
            assert(call_ok);
            auto rpc_call = static_cast<detail::RpcCall<AsyncService>*>(tag.data);

            auto active_connection = rpc_call->extract_active_connection();

            if (!tagger_.has_data(active_connection.get())) {
                active_connection->process();
            }
            void* connection_id = active_connection.get();
            active_connections_.emplace(connection_id, std::move(active_connection));
            connections_to_rpc_calls_.emplace(connection_id, rpc_call);

            rpc_call->queue_next_client_connection(service_.get(), server_queue_.get(), &tagger_);
        }
            continue;

        case detail::TagLabel::processing:
            std::cout << "processing" << std::endl;
            if (call_ok) {
                auto connection = static_cast<detail::Connection*>(tag.data);
                connection->process();
            }
            [[fallthrough]];

        case detail::TagLabel::rpc_finished:
            std::cout << "rpc_finished" << std::endl;
            // No more tags with this active_connection are left in the queue so we can delete the data
            if (tag_count == 0) {
                std::cout << "Erase" << std::endl;
                void* rpc_id = connections_to_rpc_calls_.at(tag.data);
                rpc_calls_.at(rpc_id)->disconnect() active_connections_.erase(tag.data);
            }
            break;

        } // end switch
    }
}

} // namespace net

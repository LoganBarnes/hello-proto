#pragma once

// project
#include "net/server_states.hpp"

// third-party
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

// standard
#include <unordered_map>
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
    template <typename RpcFunction, typename Callback>
    void register_rpc(RpcFunction rpc_function, Callback&& callback);
    //    template <typename BaseService, typename Request, typename Response, typename Callback>
    //    void register_unary_rpc(UnaryRpcFunction<BaseService, Request, Response> rpc_function, Callback&& callback);

    /**
     * @brief This specifies what will happen when a server-side streaming rpc call is triggered by the client.
     *
     * .............In 'echo.proto'.............
     *
     * package echo;
     *
     * service Echo {
     *     rpc EchoStream (EchoRequest) returns (stream EchoResponse);
     * }
     * .........................................
     *
     * ..............In 'echo.cpp'..............
     *
     * AsyncServer<echo::Echo> server("0.0.0.0:50051");
     *
     * // The callback that will be executed when a client calls 'EchoStream'
     * auto echo_stream_callback = [] (const echo::EchoRequest* request) {
     *                               *response = request;
     *                                return grpc::Status::OK;
     *                           };
     *
     * server.register_unary_rpc(&echo::Echo::AsyncService::RequestEchoStream, echo_stream_callback);
     * .........................................
     *
     * @param rpc_function - The server-side-streaming rpc function
     * @param callback - What to do when this RPC is first triggered <void(const Request&)>
     * @return the stream used to write Responses to the client
     */
    template <typename BaseService, typename Request, typename Response, typename Callback>
    void register_server_stream_rpc(ServerStreamRpcFunction<BaseService, Request, Response> rpc_function,
                                    Callback&& callback);

    void run();

    void operator()(detail::InactiveRpcCall& inactive_rpc);
    void operator()(detail::ActiveRpcCall& active_rpc);
    void operator()(detail::FinishedRpcCall& finished_rcp);

private:
    std::unique_ptr<typename Service::AsyncService> service_;
    std::unique_ptr<grpc::ServerCompletionQueue> server_queue_;
    std::unique_ptr<grpc::Server> server_;

    std::unordered_map<void*, std::unique_ptr<detail::ServerAction>> server_actions_;
};

template <typename Service>
AsyncServer<Service>::AsyncServer(unsigned port) : service_(std::make_unique<typename Service::AsyncService>()) {
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

    auto tag_action_pair = detail::make_inactive_rpc(service_.get(),
                                                     server_queue_.get(),
                                                     rpc_function,
                                                     std::forward<Callback>(callback));

    server_actions_.emplace(std::move(tag_action_pair));
}

template <typename Service>
template <typename BaseService, typename Request, typename Response, typename Callback>
void AsyncServer<Service>::register_server_stream_rpc(
    ServerStreamRpcFunction<BaseService, Request, Response> rpc_function, Callback&& callback) {

    server_actions_.emplace(
        detail::make_inactive_rpc(service_.get(), server_queue_.get(), rpc_function, std::forward<Callback>(callback)));
}

template <typename Service>
void AsyncServer<Service>::run() {
    void* tag;
    bool call_ok;

    while (server_queue_->Next(&tag, &call_ok)) {

#ifdef PRINT_CRAP
        std::cout << std::boolalpha << "OK: " << call_ok << std::endl;
        std::cout << "Server actions: " << server_actions_.size() << std::endl;
        for (auto& action : server_actions_) {
            assert(action.second);
            std::cout << "\t" << *action.second << std::endl;
        }
#endif
        std::cout << "Tag: " << tag << std::boolalpha << ", OK: " << call_ok << std::endl;

        if (call_ok) {
            if (server_actions_.find(tag) == server_actions_.end()) {
                std::cout << "WAT" << std::endl;
            }
            std::visit(*this, *server_actions_.at(tag));

        } else {
            std::cout << "Client disconnected 2" << std::endl;
            //            server_actions_.erase(tag);
        }
    }
}

template <typename Service>
void AsyncServer<Service>::operator()(detail::InactiveRpcCall& inactive_rpc) {
    detail::ActiveRpcCall active_rpc = inactive_rpc.get_active_rpc_call();

    std::cout << "Client connected" << std::endl;

    void* tag = active_rpc.get_tag();
    server_actions_.emplace(tag, std::make_unique<detail::ServerAction>(std::move(active_rpc)));
}

template <typename Service>
void AsyncServer<Service>::operator()(detail::ActiveRpcCall& active_rpc) {
#ifdef PRINT_CRAP
    std::cout << "Active" << std::endl;
#endif

    if (active_rpc.connection->process() == detail::ProcessState::finished) {
#ifdef PRINT_CRAP
        std::cout << "Finished" << std::endl;
#endif
        std::cout << "Client finished" << std::endl;

        detail::FinishedRpcCall finished_rpc{std::move(active_rpc.connection)};

        void* tag = finished_rpc.get_tag();
        server_actions_[tag] = std::make_unique<detail::ServerAction>(std::move(finished_rpc));
        // server_actions_.erase(active_rpc.get_tag());
    }
}

template <typename Service>
void AsyncServer<Service>::operator()(detail::FinishedRpcCall& /*finished_rcp*/) {
    std::cout << "Client disconnected" << std::endl;
    // server_actions_.erase(finished_rcp.get_tag());
}

} // namespace net

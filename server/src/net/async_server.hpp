#pragma once

// project
#include "net/server_states.hpp"

// third-party
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

// standard
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
    template <typename BaseService, typename Request, typename Response, typename Callback>
    void register_unary_rpc(UnaryRpcFunction<BaseService, Request, Response> rpc_function, Callback&& callback);

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
    detail::ServerToClientStream<Response>
    register_server_stream_rpc(ServerStreamRpcFunction<BaseService, Request, Response> rpc_function,
                               Callback&& callback);

    void run();

    void operator()(detail::InactiveRpcCall& inactive_rpc);
    void operator()(detail::ActiveRpcCall& active_rpc);

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
template <typename BaseService, typename Request, typename Response, typename Callback>
void AsyncServer<Service>::register_unary_rpc(UnaryRpcFunction<BaseService, Request, Response> unary_rpc_function,
                                              Callback&& callback) {
    static_assert(std::is_base_of<BaseService, typename Service::AsyncService>::value,
                  "BaseService must be a base class of Service");

    detail::InactiveRpcCall inactive_rpc{
        std::make_unique<detail::NonStreamingRpcCall<BaseService, Request, Response, Callback>>(service_.get(),
                                                                                                server_queue_.get(),
                                                                                                unary_rpc_function,
                                                                                                std::forward<Callback>(
                                                                                                    callback))};

    void* tag = inactive_rpc.get_tag();
    server_actions_.emplace(tag, std::make_unique<detail::ServerAction>(std::move(inactive_rpc)));
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

        auto& server_action = server_actions_.at(tag);

        if (call_ok) {
            std::visit(*this, *server_action);

        } else {
            server_actions_.erase(tag);
        }
    }
}

template <typename Service>
void AsyncServer<Service>::operator()(detail::InactiveRpcCall& inactive_rpc) {
    detail::ActiveRpcCall active_rpc = inactive_rpc.get_active_rpc_call();

    void* tag = active_rpc.get_tag();
    server_actions_.emplace(tag, std::make_unique<detail::ServerAction>(std::move(active_rpc)));
}

template <typename Service>
void AsyncServer<Service>::operator()(detail::ActiveRpcCall& active_rpc) {
#ifdef PRINT_CRAP
    std::cout << "Finished" << std::endl;
#endif

    if (active_rpc.finished) {
        server_actions_.erase(active_rpc.get_tag());
    }
}

} // namespace net

#pragma once

#include "net/connections.hpp"

#include <variant>

namespace net {

/**
 * @brief The function signature for an async service's non-streaming RPC calls
 */
template <typename Service, typename Request, typename Response>
using UnaryRpcFunction = void (Service::*)(grpc::ServerContext*,
                                           Request*,
                                           grpc::ServerAsyncResponseWriter<Response>*,
                                           grpc::CompletionQueue*,
                                           grpc::ServerCompletionQueue*,
                                           void*);

/**
 * @brief The function signature for an async service's server-side-streaming RPC calls
 */
template <typename Service, typename Request, typename Response>
using ServerStreamRpcFunction = void (Service::*)(grpc::ServerContext* context,
                                                  Request*,
                                                  grpc::ServerAsyncWriter<Response>*,
                                                  grpc::CompletionQueue*,
                                                  grpc::ServerCompletionQueue*,
                                                  void*);

namespace detail {

struct RpcCall {
    virtual ~RpcCall() = 0;
    virtual std::unique_ptr<Connection> extract_active_connection() = 0;
};

inline RpcCall::~RpcCall() = default;

template <typename Service, typename Request, typename Response, typename Callback>
struct UnaryRpcCall : RpcCall {
    explicit UnaryRpcCall(Service* service,
                          grpc::ServerCompletionQueue* server_queue,
                          UnaryRpcFunction<Service, Request, Response> unary_rpc_function,
                          Callback&& callback)
        : service_(service), server_queue_(server_queue), unary_rpc_function_(unary_rpc_function), callback_(callback) {
        create_call_and_add_to_server_queue();
    }

    ~UnaryRpcCall() override = default;

    std::unique_ptr<Connection> extract_active_connection() override {
        // Save the current active connection so we can modify and return it
        auto active_connection = std::move(connection_);

        // Create a new connection that hasn't been processed and add it to the queue
        create_call_and_add_to_server_queue();

        // Process the rpc call however the user specified in their callback
        Response response;
        grpc::Status status = callback_(active_connection->request, &response);
        active_connection->responder.Finish(response, status, active_connection->get_tag());
        std::cout << "Unary finish: " << active_connection->get_tag() << std::endl;

        // Return the processed connection
        return std::move(active_connection);
    }

    void create_call_and_add_to_server_queue() {
        // Add a new connection that is waiting to be activated
        connection_ = std::make_unique<UnaryRpcConnection<Request, Response>>();

        std::cout << "Unary: " << this << std::endl;
        (service_->*unary_rpc_function_)(&connection_->context,
                                         &connection_->request,
                                         &connection_->responder,
                                         server_queue_,
                                         server_queue_,
                                         this);

#ifdef PRINT_CRAP
        std::cout << "Created" << std::endl;
#endif
    }

private:
    Service* service_;
    grpc::ServerCompletionQueue* server_queue_;

    UnaryRpcFunction<Service, Request, Response> unary_rpc_function_;
    Callback callback_;

    std::unique_ptr<UnaryRpcConnection<Request, Response>> connection_;
};

template <typename Service, typename Request, typename Response, typename Callback>
struct ServerStreamRpcCall : RpcCall {
    explicit ServerStreamRpcCall(Service* service,
                                 grpc::ServerCompletionQueue* server_queue,
                                 ServerStreamRpcFunction<Service, Request, Response> server_stream_rpc_function,
                                 Callback&& callback)
        : service_(service)
        , server_queue_(server_queue)
        , server_stream_rpc_function_(server_stream_rpc_function)
        , callback_(callback) {
        create_call_and_add_to_server_queue();
    }

    ~ServerStreamRpcCall() override = default;

    std::unique_ptr<Connection> extract_active_connection() override {
        // Save the current active connection so we can modify and return it
        auto active_connection = std::move(connection_);

        // Create a new connection that hasn't been processed and add it to the queue
        create_call_and_add_to_server_queue();

        // Process the rpc call however the user specified in their callback
        callback_(active_connection->request, &active_connection->stream);

        // Return the processed connection
        return std::move(active_connection);
    }

    void create_call_and_add_to_server_queue() {
        // Add a new connection that is waiting to be activated
        connection_ = std::make_unique<ServerStreamRpcConnection<Request, Response>>();

        connection_->context.AsyncNotifyWhenDone(connection_->get_tag());

        std::cout << "Stream: " << this << std::endl;
        (service_->*server_stream_rpc_function_)(&connection_->context,
                                                 &connection_->request,
                                                 &connection_->responses.responder,
                                                 server_queue_,
                                                 server_queue_,
                                                 this);

#ifdef PRINT_CRAP
        std::cout << "Created" << std::endl;
#endif
    }

private:
    Service* service_;
    grpc::ServerCompletionQueue* server_queue_;

    ServerStreamRpcFunction<Service, Request, Response> server_stream_rpc_function_;
    Callback callback_;

    std::unique_ptr<ServerStreamRpcConnection<Request, Response>> connection_;
};

/**
 * @brief
 */
struct FinishedRpcCall {
    std::unique_ptr<Connection> connection;
    void* get_tag() { return connection->get_tag(); }
};

/**
 * @brief
 */
struct ActiveRpcCall {
    std::unique_ptr<Connection> connection;

    void* get_tag() { return connection->get_tag(); }
};

/**
 * @brief
 */
struct InactiveRpcCall {
    std::unique_ptr<RpcCall> rpc_call;

    ActiveRpcCall get_active_rpc_call() {
        std::unique_ptr<Connection> connection = rpc_call->extract_active_connection();
        return {std::move(connection)};
    }

    void* get_tag() { return rpc_call.get(); }
};

/**
 * @brief
 */
using ServerAction = std::variant<InactiveRpcCall, ActiveRpcCall, FinishedRpcCall>;

template <typename Service, typename BaseService, typename Request, typename Response, typename Callback>
std::pair<void*, std::unique_ptr<ServerAction>>
make_inactive_rpc(Service* service,
                  grpc::ServerCompletionQueue* server_queue,
                  UnaryRpcFunction<BaseService, Request, Response> unary_rpc_function,
                  Callback&& callback) {

    static_assert(std::is_base_of<BaseService, Service>::value, "BaseService must be a base class of Service");

    detail::InactiveRpcCall inactive_rpc{
        std::make_unique<detail::UnaryRpcCall<BaseService, Request, Response, Callback>>(service,
                                                                                         server_queue,
                                                                                         unary_rpc_function,
                                                                                         std::forward<Callback>(
                                                                                             callback))};

    void* tag = inactive_rpc.get_tag();
    return std::make_pair(tag, std::make_unique<detail::ServerAction>(std::move(inactive_rpc)));
}

template <typename Service, typename BaseService, typename Request, typename Response, typename Callback>
std::pair<void*, std::unique_ptr<ServerAction>>
make_inactive_rpc(Service* service,
                  grpc::ServerCompletionQueue* server_queue,
                  ServerStreamRpcFunction<BaseService, Request, Response> server_stream_rpc_function,
                  Callback&& callback) {

    static_assert(std::is_base_of<BaseService, Service>::value, "BaseService must be a base class of Service");

    detail::InactiveRpcCall inactive_rpc{std::make_unique<
        detail::ServerStreamRpcCall<BaseService, Request, Response, Callback>>(service,
                                                                               server_queue,
                                                                               server_stream_rpc_function,
                                                                               std::forward<Callback>(callback))};

    void* tag = inactive_rpc.get_tag();
    return std::make_pair(tag, std::make_unique<detail::ServerAction>(std::move(inactive_rpc)));
}

} // namespace detail
} // namespace net

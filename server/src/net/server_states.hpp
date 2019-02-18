#pragma once

#include "net/connections.hpp"

#include <variant>

namespace net {

template <typename Service, typename Request, typename Response, template <typename> class Writer>
using RpcFunction = void (Service::*)(
    grpc::ServerContext*, Request*, Writer<Response>*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void*);

/**
 * @brief The function signature for an async service's non-streaming RPC calls
 */
template <typename Service, typename Request, typename Response>
using UnaryRpcFunction = RpcFunction<Service, Request, Response, grpc::ServerAsyncResponseWriter>;

/**
 * @brief The function signature for an async service's server-side-streaming RPC calls
 */
template <typename Service, typename Request, typename Response>
using ServerStreamRpcFunction = RpcFunction<Service, Request, Response, grpc::ServerAsyncWriter>;

namespace detail {

template <typename Service>
struct RpcCallHandle {
    virtual ~RpcCallHandle() = 0;
    virtual void queue_next_client_connection(Service* service, grpc::ServerCompletionQueue* queue, Tagger* tagger) = 0;
    virtual std::unique_ptr<Connection> extract_active_connection() = 0;
    virtual void disconnect(void* server_to_client_stream) = 0;
};

template <typename Service>
inline RpcCallHandle<Service>::~RpcCallHandle() = default;

/**
 * @brief
 * @tparam Service
 * @tparam Request
 * @tparam Response
 * @tparam Callback
 */
template <typename Service,
          typename BaseService,
          typename Request,
          typename Response,
          template <typename> class Writer,
          typename RpcConnection,
          typename ConnectCallback,
          typename DisconnectCallback>
struct RpcCall : RpcCallHandle<Service> {

    using RpcFunc = RpcFunction<BaseService, Request, Response, Writer>;

    RpcCall(RpcFunc rpc_function, ConnectCallback&& connect_callback, DisconnectCallback&& disconnect_callback)
        : rpc_function_(rpc_function), connect_callback_(connect_callback), disconnect_callback_(disconnect_callback) {}

    void queue_next_client_connection(Service* service, grpc::ServerCompletionQueue* queue, Tagger* tagger) override {
        connection_ = std::make_unique<RpcConnection>(tagger);

        connection_->context.AsyncNotifyWhenDone(tagger->make_tag(TagLabel::rpc_finished, connection_.get()));

        (service->*rpc_function_)(&connection_->context,
                                  &request_,
                                  &connection_->responder,
                                  queue,
                                  queue,
                                  tagger->make_tag(TagLabel::rpc_call_requested_by_client, this));
    }

    std::unique_ptr<Connection> extract_active_connection() override {
        connection_->status = connect_callback_(request_, &connection_->response);
        return std::move(connection_);
    }

    void disconnect(void* server_to_client_stream) override {
        disconnect_callback_(static_cast<ServerToClientStream<Response>*>(server_to_client_stream));
    }

private:
    RpcFunc rpc_function_;
    Request request_;
    ConnectCallback connect_callback_;
    DisconnectCallback disconnect_callback_;
    std::unique_ptr<RpcConnection> connection_;
};
//
//template <typename Service,
//          typename BaseService,
//          typename Request,
//          typename Response,
//          typename ConnectCallback,
//          typename DisconnectCallback>
//struct ServerStreamRpcCall : RpcCallHandle<Service> {
//
//    ServerStreamRpcFunction<BaseService, Request, Response> server_stream_rpc_function;
//    Request request;
//    ConnectCallback connect_callback;
//    DisconnectCallback disconnect_callback;
//    std::unique_ptr<ServerStreamRpcConnection<Response>> connection = nullptr;
//
//    ServerStreamRpcCall(ServerStreamRpcFunction<BaseService, Request, Response> rpc_function,
//                        ConnectCallback&& callback_function,
//                        DisconnectCallback&& disconnect_callback_function)
//        : server_stream_rpc_function(rpc_function), connect_callback(callback_function), connection(nullptr) {}
//
//    void queue_next_client_connection(Service* service, grpc::ServerCompletionQueue* queue, Tagger* tagger) override {
//
//        connection = std::make_unique<ServerStreamRpcConnection<Response>>(tagger);
//
//        connection->context.AsyncNotifyWhenDone(tagger->make_tag(TagLabel::rpc_finished, connection.get()));
//
//        (service->*server_stream_rpc_function)(&connection->context,
//                                               &request,
//                                               &connection->responder,
//                                               queue,
//                                               queue,
//                                               tagger->make_tag(TagLabel::rpc_call_requested_by_client, this));
//    }
//
//    std::unique_ptr<Connection> extract_active_connection() override {
//        connect_callback(request, &connection->stream);
//        return std::move(connection);
//    }
//
//    void disconnect(void* data) override { disconnect_callback(static_cast<ServerToClientStream<Response>*>(data)); }
//};

template <typename Service,
          typename BaseService,
          typename Request,
          typename Response,
          typename ConnectCallback,
          typename DisconnectCallback>
std::unique_ptr<detail::RpcCallHandle<Service>>
make_rpc_call_handle(UnaryRpcFunction<BaseService, Request, Response> unary_rpc_function, ConnectCallback&& callback) {

    static_assert(std::is_base_of<BaseService, Service>::value, "BaseService must be a base class of Service");

    using UnaryRpc = detail::UnaryRpcCall<Service, BaseService, Request, Response, ConnectCallback>;
    return std::make_unique<UnaryRpc>(unary_rpc_function, std::forward<ConnectCallback>(callback));
}

template <typename Service,
          typename BaseService,
          typename Request,
          typename Response,
          typename ConnectCallback,
          typename DisconnectCallback>
std::unique_ptr<detail::RpcCallHandle<Service>>
make_rpc_call_handle(ServerStreamRpcFunction<BaseService, Request, Response> server_stream_rpc_function,
                     ConnectCallback&& callback) {

    static_assert(std::is_base_of<BaseService, Service>::value, "BaseService must be a base class of Service");

    using ServerStreamRpc = detail::ServerStreamRpcCall<Service, BaseService, Request, Response, ConnectCallback>;
    return std::make_unique<ServerStreamRpc>(server_stream_rpc_function, std::forward<ConnectCallback>(callback));
}

} // namespace detail
} // namespace net

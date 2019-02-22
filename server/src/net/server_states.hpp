#pragma once

// project
#include "net/connections.hpp"
#include "testing/testing.hpp"

// third-party
#include <grpcpp/completion_queue.h>

#ifdef DOCTEST_LIBRARY_INCLUDED
#include <google/protobuf/empty.pb.h>
#include <testing/echo.grpc.pb.h>

namespace testing {

struct EmptyConnect {
    grpc::Status operator()(const testing::proto::EchoRequest&, testing::proto::EchoResponse*) const {
        return grpc::Status::OK;
    }
    std::unique_ptr<grpc::Status> operator()(const testing::proto::EchoRequest&,
                                             net::ServerToClientStream<testing::proto::EchoResponse>*) const {
        return nullptr;
    }
    void operator()(const google::protobuf::Empty&, net::ServerToClientStream<testing::proto::EchoResponse>*) const {}
};

} // namespace testing
#endif

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

struct EmptyDisconnect {
    void operator()(void*) const {}
};

template <typename Service>
struct RpcCallHandle {
    virtual ~RpcCallHandle() = 0;
    virtual void queue_next_client_connection(Service* service, grpc::ServerCompletionQueue* queue, Tagger* tagger) = 0;
    virtual std::unique_ptr<Connection> extract_active_connection() = 0;
    virtual void disconnect(void* connection) = 0;
};

template <typename Service>
inline RpcCallHandle<Service>::~RpcCallHandle() = default;

/**
 * @brief
 * @tparam Service
 * @tparam BaseService
 * @tparam Request
 * @tparam Response
 * @tparam Writer
 * @tparam RpcConnection
 * @tparam ConnectCallback
 * @tparam DisconnectCallback
 */
template <typename Service,
          typename BaseService,
          typename Request,
          typename Response,
          template <typename> class Writer,
          typename RpcConnection,
          typename ConnectCallback,
          typename DisconnectCallback>
class RpcCall : public RpcCallHandle<Service> {
public:
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

    void disconnect(void* connection) override { disconnect_callback_(connection); }

private:
    RpcFunc rpc_function_;
    Request request_;
    ConnectCallback connect_callback_;
    DisconnectCallback disconnect_callback_;
    std::unique_ptr<RpcConnection> connection_;
};

#ifdef DOCTEST_LIBRARY_INCLUDED
template class RpcCall<testing::proto::Echo::AsyncService,
                       testing::proto::Echo::AsyncService,
                       testing::proto::EchoRequest,
                       testing::proto::EchoResponse,
                       grpc::ServerAsyncResponseWriter,
                       UnaryRpcConnection<testing::proto::EchoResponse>,
                       testing::EmptyConnect,
                       EmptyDisconnect>;

template class RpcCall<testing::proto::Echo::AsyncService,
                       testing::proto::Echo::AsyncService,
                       testing::proto::EchoRequest,
                       testing::proto::EchoResponse,
                       grpc::ServerAsyncWriter,
                       ServerStreamRpcConnection<testing::proto::EchoResponse>,
                       testing::EmptyConnect,
                       EmptyDisconnect>;
#endif

/**
 * @brief
 * @tparam Service
 * @tparam BaseService
 * @tparam Request
 * @tparam Response
 * @tparam ConnectCallback
 * @tparam DisconnectCallback
 * @param unary_rpc_function
 * @param connect_callback
 * @param disconnect_callback
 * @return
 */

template <typename Service,
          typename BaseService,
          typename Request,
          typename Response,
          typename ConnectCallback,
          typename DisconnectCallback>
std::unique_ptr<detail::RpcCallHandle<Service>>
make_rpc_call_handle(UnaryRpcFunction<BaseService, Request, Response> unary_rpc_function,
                     ConnectCallback&& connect_callback,
                     DisconnectCallback&& disconnect_callback) {

    static_assert(std::is_base_of<BaseService, Service>::value, "BaseService must be a base class of Service");

    using UnaryRpc = detail::RpcCall<Service,
                                     BaseService,
                                     Request,
                                     Response,
                                     grpc::ServerAsyncResponseWriter,
                                     UnaryRpcConnection<Response>,
                                     ConnectCallback,
                                     DisconnectCallback>;

    return std::make_unique<UnaryRpc>(unary_rpc_function,
                                      std::forward<ConnectCallback>(connect_callback),
                                      std::forward<DisconnectCallback>(disconnect_callback));
}

#ifdef DOCTEST_LIBRARY_INCLUDED
template std::unique_ptr<detail::RpcCallHandle<testing::proto::Echo::AsyncService>>
make_rpc_call_handle<testing::proto::Echo::AsyncService,
                     testing::proto::Echo::AsyncService,
                     testing::proto::EchoRequest,
                     testing::proto::EchoResponse,
                     testing::EmptyConnect,
                     EmptyDisconnect>(
    UnaryRpcFunction<testing::proto::Echo::AsyncService, testing::proto::EchoRequest, testing::proto::EchoResponse>,
    testing::EmptyConnect&&,
    EmptyDisconnect&&);
#endif

/**
 * @brief
 * @tparam Service
 * @tparam BaseService
 * @tparam Request
 * @tparam Response
 * @tparam ConnectCallback
 * @tparam DisconnectCallback
 * @param server_stream_rpc_function
 * @param connect_callback
 * @param disconnect_callback
 * @return
 */
template <typename Service,
          typename BaseService,
          typename Request,
          typename Response,
          typename ConnectCallback,
          typename DisconnectCallback>
std::unique_ptr<detail::RpcCallHandle<Service>>
make_rpc_call_handle(ServerStreamRpcFunction<BaseService, Request, Response> server_stream_rpc_function,
                     ConnectCallback&& connect_callback,
                     DisconnectCallback&& disconnect_callback) {

    static_assert(std::is_base_of<BaseService, Service>::value, "BaseService must be a base class of Service");

    auto connect_callback_wrapper
        = [connect_callback](const Request& request,
                             ServerToClientStream<Response>* stream) -> std::unique_ptr<grpc::Status> {
        connect_callback(request, stream);
        return stream->status();
    };

    auto disconnect_callback_wrapper = [disconnect_callback{disconnect_callback}](void* connection) {
        auto server_stream_connection = static_cast<ServerStreamRpcConnection<Response>*>(connection);
        disconnect_callback(&server_stream_connection->response);
    };

    using ServerStreamRpc = detail::RpcCall<Service,
                                            BaseService,
                                            Request,
                                            Response,
                                            grpc::ServerAsyncWriter,
                                            ServerStreamRpcConnection<Response>,
                                            decltype(connect_callback_wrapper),
                                            decltype(disconnect_callback_wrapper)>;

    return std::make_unique<ServerStreamRpc>(server_stream_rpc_function,
                                             std::move(connect_callback_wrapper),
                                             std::move(disconnect_callback_wrapper));
}

#ifdef DOCTEST_LIBRARY_INCLUDED
template std::unique_ptr<detail::RpcCallHandle<testing::proto::Echo::AsyncService>>
make_rpc_call_handle<testing::proto::Echo::AsyncService,
                     testing::proto::Echo::AsyncService,
                     google::protobuf::Empty,
                     testing::proto::EchoResponse,
                     testing::EmptyConnect,
                     EmptyDisconnect>(
    ServerStreamRpcFunction<testing::proto::Echo::AsyncService, google::protobuf::Empty, testing::proto::EchoResponse>,
    testing::EmptyConnect&&,
    EmptyDisconnect&&);
#endif

} // namespace detail
} // namespace net

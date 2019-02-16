#pragma once

#include <grpcpp/server.h>
#include <grpcpp/server_context.h>

//#define PRINT_CRAP

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

namespace detail {

//template <typename Response>
//class ServerToClientStream {
//public:
//    virtual ~ServerToClientStream() = 0;
//    virtual bool write(const Response& update) = 0;
//};
//
//template <typename Response>
//ServerToClientStream<Response>::~ServerToClientStream() = default;

struct Connection {
    virtual ~Connection() = 0;
    virtual bool write_or_finish(void* tag) = 0;
};

inline Connection::~Connection() = default;

template <typename Request, typename Response>
struct UnaryRpcConnection : Connection {
    grpc::ServerContext context;
    Request request;
    Response response;
    grpc::Status status;
    grpc::ServerAsyncResponseWriter<Response> responder;

    UnaryRpcConnection() : responder(&context) {}
    ~UnaryRpcConnection() override = default;

    bool write_or_finish(void* tag) override {
        responder.Finish(response, status, tag);
#ifdef PRINT_CRAP
        std::cout << "Processed" << std::endl;
#endif
        return true;
    }
};

} // namespace detail
} // namespace net

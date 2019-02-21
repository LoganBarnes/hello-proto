#pragma once

// project
#include "net/tagger.hpp"
#include "testing/testing.hpp"

// thirdparty
#include <grpcpp/server_context.h>

// standard
#include <queue>

#ifdef DOCTEST_LIBRARY_INCLUDED
#include <testing/echo.grpc.pb.h>
#endif

namespace net {

/*
 * Forward declaration
 */
namespace detail {
template <typename Response>
struct ServerStreamRpcConnection;
} // namespace detail

template <typename Response>
class ServerToClientStream {
public:
    explicit ServerToClientStream(detail::ServerStreamRpcConnection<Response>* connection);

    /**
     * @brief
     */
    void write(const Response& response);

    /**
     * @brief Only call once. After calling this function the stream should not be used anymore.
     */
    void finish(const grpc::Status& status);

    std::unique_ptr<grpc::Status> status();

private:
    detail::ServerStreamRpcConnection<Response>* connection_;
};

#ifdef DOCTEST_LIBRARY_INCLUDED
template class ServerToClientStream<testing::proto::EchoResponse>;
#endif

namespace detail {

enum class ProcessState { processing, finished };

struct Connection {
    virtual ~Connection() = 0;
    virtual void process() = 0;
    virtual void cancel() = 0;
};

inline Connection::~Connection() = default;

/**
 * @brief
 * @tparam Request
 * @tparam Response
 */
template <typename Response>
struct UnaryRpcConnection : Connection {
    Tagger* tagger;
    grpc::ServerContext context;
    Response response;
    grpc::Status status;
    grpc::ServerAsyncResponseWriter<Response> responder;
    ProcessState state;

    explicit UnaryRpcConnection(Tagger* tgr) : tagger(tgr), responder(&context), state(ProcessState::processing) {}
    ~UnaryRpcConnection() override = default;

    void process() override {
        if (state == ProcessState::processing) {
            responder.Finish(response, status, tagger->make_tag(TagLabel::processing, this));
            state = ProcessState::finished;
        }
    }

    void cancel() override { context.TryCancel(); }
};

#ifdef DOCTEST_LIBRARY_INCLUDED
template struct UnaryRpcConnection<testing::proto::EchoResponse>;
#endif

/**
 * @brief
 * @tparam Response
 */
template <typename Response>
struct ServerStreamRpcConnection : Connection {
    Tagger* tagger;
    grpc::ServerContext context;
    std::unique_ptr<grpc::Status> status;
    grpc::ServerAsyncWriter<Response> responder;
    std::queue<Response> queue;
    ProcessState state;
    ServerToClientStream<Response> response;

    explicit ServerStreamRpcConnection(Tagger* tgr)
        : tagger(tgr), responder(&context), state(ProcessState::processing), response(this) {}

    ~ServerStreamRpcConnection() override = default;

    void process() override {
        if (state == ProcessState::finished) {
            return;
        }

        // The current state has just been processed so we can pop it from the queue
        if (!queue.empty()) {
            queue.pop();
        }

        // If more responses need to be processed then write the next one to the stream
        if (!queue.empty()) {
            responder.Write(queue.front(), tagger->make_tag(detail::TagLabel::processing, this));
        }

        // If the user has finished the with stream and set the status then call 'Finish'
        // on the stream. We will only reach this point if the queue is already empty.
        else if (status != nullptr) {
            responder.Finish(*status, tagger->make_tag(detail::TagLabel::processing, this));
            state = ProcessState::finished;
        }
    }

    void cancel() override { context.TryCancel(); }
};

#ifdef DOCTEST_LIBRARY_INCLUDED
template struct ServerStreamRpcConnection<testing::proto::EchoResponse>;
#endif

} // namespace detail

template <typename Response>
ServerToClientStream<Response>::ServerToClientStream(detail::ServerStreamRpcConnection<Response>* connection)
    : connection_(connection) {}

template <typename Response>
void ServerToClientStream<Response>::write(const Response& response) {
    if (connection_->state == detail::ProcessState::finished) {
        return;
    }

    // If there are no responses queued then write directly to the stream
    if (connection_->queue.empty()) {
        connection_->responder.Write(response,
                                     connection_->tagger->make_tag(detail::TagLabel::processing, connection_));
    }
    // Queue the current response until it is processed by the server queue
    connection_->queue.push(response);
}

template <typename Response>
void ServerToClientStream<Response>::finish(const grpc::Status& status) {
    if (connection_->state == detail::ProcessState::finished) {
        return;
    }

    // If there are no responses queued then write directly to the stream
    if (connection_->queue.empty()) {
        connection_->responder.Finish(status, connection_->tagger->make_tag(detail::TagLabel::processing, connection_));
        connection_->state = detail::ProcessState::finished;
    } else {
        // Otherwise save the status to be processed when there are no more responses queued
        connection_->status = std::make_unique<grpc::Status>(status);
    }
}

template <typename Response>
std::unique_ptr<grpc::Status> ServerToClientStream<Response>::status() {
    return std::move(connection_->status);
}

} // namespace net

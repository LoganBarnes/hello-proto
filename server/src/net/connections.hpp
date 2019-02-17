#pragma once

#include <grpcpp/server.h>
#include <grpcpp/server_context.h>

#include <queue>

//#define PRINT_CRAP

namespace net {

/*
 * Forward declaration
 */
namespace detail {
template <typename Response>
struct ServerStreamResponses;
} // namespace detail

template <typename Response>
class ServerToClientStream {
public:
    ServerToClientStream(void* tag, detail::ServerStreamResponses<Response>* responses);

    /**
     * @brief
     */
    void write(const Response& response);

    /**
     * @brief Only call once. After calling this function the stream should not be used anymore.
     */
    void finish(const grpc::Status& status);

private:
    void* tag_;
    detail::ServerStreamResponses<Response>* responses_;
};

namespace detail {

enum class ProcessState { processing, finished };

struct Connection {
    virtual ~Connection() = 0;
    virtual void* get_tag() = 0;
    virtual ProcessState process() = 0;
};

inline Connection::~Connection() = default;

/**
 * @brief
 * @tparam Request
 * @tparam Response
 */
template <typename Request, typename Response>
struct UnaryRpcConnection : Connection {
    grpc::ServerContext context;
    Request request;
    grpc::ServerAsyncResponseWriter<Response> responder;

    UnaryRpcConnection() : responder(&context) {}
    ~UnaryRpcConnection() override = default;

    void* get_tag() override { return this; }
    ProcessState process() override { return ProcessState::finished; }
};

/**
 * @brief
 * @tparam Response
 */
template <typename Response>
struct ServerStreamResponses {
    grpc::ServerAsyncWriter<Response> responder;
    std::unique_ptr<grpc::Status> status;
    std::queue<Response> queue;
    ProcessState state;

    explicit ServerStreamResponses(grpc::ServerContext* context)
        : responder(context), state(ProcessState::processing) {}
};

/**
 * @brief
 * @tparam Request
 * @tparam Response
 */
template <typename Request, typename Response>
struct ServerStreamRpcConnection : Connection {
    grpc::ServerContext context;
    Request request;
    ServerStreamResponses<Response> responses;
    ServerToClientStream<Response> stream;

    ServerStreamRpcConnection() : responses(&context), stream(get_tag(), &responses) {}
    ~ServerStreamRpcConnection() override = default;

    void* get_tag() override { return this; }

    ProcessState process() override {
        // The current state has just been processed so we can pop it from the queue
        if (!responses.queue.empty()) {
            responses.queue.pop();
        }

        // If more responses need to be processed then write the next one to the stream
        if (!responses.queue.empty()) {
            responses.responder.Write(responses.queue.front(), get_tag());
            std::cout << "Stream write: " << get_tag() << std::endl;
            return ProcessState::processing;
        }

        // If the user has finished the with stream and set the status then call 'Finish'
        // on the stream. We will only reach this point if the queue is already empty.
        if (responses.status != nullptr) {
            responses.responder.Finish(*responses.status, get_tag());
            std::cout << "Stream finish: " << get_tag() << std::endl;
            responses.status = nullptr; // Don't call finish again.
            responses.state = ProcessState::finished;
            return ProcessState::processing;
        }

        return responses.state;
    }
};

} // namespace detail

template <typename Response>
ServerToClientStream<Response>::ServerToClientStream(void* tag, detail::ServerStreamResponses<Response>* responses)
    : tag_(tag), responses_(responses) {}

template <typename Response>
void ServerToClientStream<Response>::write(const Response& response) {
    // If there are no responses queued then write directly to the stream
    if (responses_->queue.empty()) {
        responses_->responder.Write(response, tag_);
        std::cout << "Stream write: " << tag_ << std::endl;
    }
    // Queue the current response until it is processed by the server queue
    responses_->queue.push(response);
}

template <typename Response>
void ServerToClientStream<Response>::finish(const grpc::Status& status) {
    // If there are no responses queued then write directly to the stream
    if (responses_->queue.empty()) {
        responses_->responder.Finish(status, tag_);
        std::cout << "Stream finish: " << tag_ << std::endl;
        responses_->state = detail::ProcessState::finished;
    } else {
        // Otherwise save the status to be processed when there are no more responses queued
        responses_->status = std::make_unique<grpc::Status>(status);
    }
}

} // namespace net

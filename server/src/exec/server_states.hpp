#pragma once

#include "connections.hpp"

#include <variant>

namespace net {
namespace detail {

struct RpcCall {
    virtual ~RpcCall() = 0;
    virtual std::unique_ptr<Connection> extract_active_connection() = 0;
};

inline RpcCall::~RpcCall() = default;

template <typename Service, typename Request, typename Response, typename Callback>
struct NonStreamingRpcCall : RpcCall {
    explicit NonStreamingRpcCall(Service* service,
                                 grpc::ServerCompletionQueue* server_queue,
                                 UnaryRpcFunction<Service, Request, Response> unary_rpc_function,
                                 Callback&& callback)
        : service_(service), server_queue_(server_queue), unary_rpc_function_(unary_rpc_function), callback_(callback) {
        create_call_and_add_to_server_queue();
    }

    ~NonStreamingRpcCall() override = default;

    std::unique_ptr<Connection> extract_active_connection() override {
        // Save the current active connection so we can modify and return it
        auto active_connection = std::move(connection_);

        // Create a new connection that hasn't been processed and add it to the queue
        create_call_and_add_to_server_queue();

        // Process the rpc call however the user specified in their callback
        Response response;
        active_connection->status = callback_(active_connection->request, &response);

        // Return the processed connection
        return std::move(active_connection);
    }

    void create_call_and_add_to_server_queue() {
        // Add a new connection that is waiting to be activated
        connection_ = std::make_unique<UnaryRpcConnection<Request, Response>>();

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

/**
 * @brief
 */
struct ActiveRpcCall {
    std::unique_ptr<Connection> connection;
    bool finished = false;

    void* get_tag() const { return connection.get(); }
    std::string get_type() const { return "active"; }
};

/**
 * @brief
 */
struct InactiveRpcCall {
    std::unique_ptr<RpcCall> rpc_call;

    ActiveRpcCall get_active_rpc_call() {
        std::unique_ptr<Connection> connection = rpc_call->extract_active_connection();
        bool finished = connection->write_or_finish(connection.get());
        return {std::move(connection), finished};
    }

    void* get_tag() const { return rpc_call.get(); }
    std::string get_type() const { return "inactive"; }
};

/**
 * @brief
 */
using ServerAction = std::variant<InactiveRpcCall, ActiveRpcCall>;

inline ::std::ostream& operator<<(::std::ostream& os, const ServerAction& server_action) {
    std::visit([&os](const auto& action) { os << action.get_tag() << ": " << action.get_type(); }, server_action);
    return os;
}

} // namespace detail
} // namespace net

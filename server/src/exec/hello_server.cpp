// project
#include "net/async_server.hpp"

// generated
#include <hello/hello.grpc.pb.h>

// system
#include <iostream>
#include <net/server_to_client_stream.hpp>

namespace hello {

class HelloServer {
public:
    explicit HelloServer(unsigned port) : server_(port) {

        server_.register_rpc(&proto::Greeter::AsyncService::RequestSayHello,
                             [this](const proto::HelloRequest& request, proto::HelloResponse* response) {
                                 return say_hello(request, response);
                             });

        server_.register_rpc(&proto::Greeter::AsyncService::RequestGetAllTransactions,
                             [this](const google::protobuf::Empty& request,
                                    net::ServerToClientStream<proto::HelloTransaction>* stream) {
                                 get_all_transactions(request, stream);
                             });

        server_.register_rpc(&proto::Greeter::AsyncService::RequestMaybeSayHello,
                             [this](const proto::HelloRequest& request, google::protobuf::Empty* response) {
                                 return maybe_say_hello(request, response);
                             });

        server_.register_rpc(&proto::Greeter::AsyncService::RequestGetTransactionUpdates,
                             [this](const google::protobuf::Empty& /*request*/,
                                    net::ServerToClientStream<proto::HelloTransaction>* stream) {
                                 client_streams_.emplace(stream);
                             },
                             [this](void* stream) {
                                 auto stream_ptr
                                     = static_cast<net::ServerToClientStream<proto::HelloTransaction>*>(stream);
                                 assert(client_streams_.find(stream_ptr) != client_streams_.end());
                                 client_streams_.erase(stream_ptr);
                             });
    }

    void run() { server_.run(); }

private:
    net::AsyncServer<proto::Greeter> server_;
    std::vector<proto::HelloTransaction> transactions_;
    std::unordered_set<net::ServerToClientStream<proto::HelloTransaction>*> client_streams_;

    grpc::Status say_hello(const proto::HelloRequest& request, proto::HelloResponse* response) {

        response->set_message("Hello, " + request.name() + "!");

        proto::HelloTransaction transaction;
        *transaction.mutable_request() = request;
        *transaction.mutable_response() = *response;

        transactions_.emplace_back(std::move(transaction));

        return grpc::Status::OK;
    }

    void get_all_transactions(const google::protobuf::Empty& /*request*/,
                              net::ServerToClientStream<proto::HelloTransaction>* stream) {

        for (const auto& transaction : transactions_) {
            stream->write(transaction);
        }

        stream->finish(grpc::Status::OK);
    }

    grpc::Status maybe_say_hello(const proto::HelloRequest& request, google::protobuf::Empty* /*response*/) {

        proto::HelloTransaction transaction;
        *transaction.mutable_request() = request;
        transaction.mutable_response()->set_message("Hello, " + request.name() + "!");

        for (net::ServerToClientStream<proto::HelloTransaction>* client_stream : client_streams_) {
            client_stream->write(transaction);
        }

        transactions_.emplace_back(std::move(transaction));

        return grpc::Status::OK;
    }
};

} // namespace hello

int main(int argc, const char* argv[]) {

    unsigned port = 9090u;

    if (argc > 1) {
        port = static_cast<unsigned>(std::stoul(argv[1]));
    }

    hello::HelloServer hello_server(/*port=*/port);
    hello_server.run();

    return 0;
}

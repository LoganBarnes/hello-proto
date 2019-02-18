#if 0
// generated
#include <hello/hello.grpc.pb.h>

// third-party
#include <grpc++/server.h>
#include <grpc++/server_builder.h>

// system
#include <iostream>
#include <thread>

namespace hello {

class HelloService : public hello::proto::Greeter::Service {
public:
    grpc::Status SayHello(grpc::ServerContext* /*context*/,
                          const proto::HelloRequest* request,
                          proto::HelloResponse* response) override {

        std::cout << "Recieved request: " << request->ShortDebugString() << std::endl;

        response->set_message("Hello, " + request->name() + "!");

        std::cout << "Sending response: " << response->ShortDebugString() << '\n' << std::endl;

        proto::HelloTransaction transaction;
        *transaction.mutable_request() = *request;
        *transaction.mutable_response() = *response;

        std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);
        transactions_.emplace_back(std::move(transaction));

        return grpc::Status::OK;
    }

    grpc::Status GetAllTransactions(grpc::ServerContext* /*context*/,
                                    const google::protobuf::Empty* /*request*/,
                                    grpc::ServerWriter<hello::proto::HelloTransaction>* writer) override {

        std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);

        for (const proto::HelloTransaction& transaction : transactions_) {

            if (!writer->Write(transaction)) {
                break; // Stream is broken
            }
        }

        return grpc::Status::OK;
    }

private:
    std::mutex transaction_mutex_;
    std::vector<hello::proto::HelloTransaction> transactions_;
};

} // namespace hello

int main(int argc, const char* argv[]) {

    std::string server_address = "0.0.0.0:9090";

    if (argc > 1) {
        server_address = argv[1];
    }

    hello::HelloService service;

    grpc::ServerBuilder builder;
    builder.RegisterService(&service);
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();

    std::cout << "Server running at " << server_address << std::endl;
    server->Wait();

    return 0;
}
#else
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
                                 this->get_all_transactions(request, stream);
                             });

        server_.register_rpc(&proto::Greeter::AsyncService::RequestMaybeSayHello,
                             [this](const proto::HelloRequest& request, google::protobuf::Empty* response) {
                                 return maybe_say_hello(request, response);
                             });

        // TODO: add callback for stream disconnect
        server_.register_rpc(&proto::Greeter::AsyncService::RequestGetTransactionUpdates,
                             [this](const google::protobuf::Empty& /*request*/,
                                    net::ServerToClientStream<proto::HelloTransaction>* stream) {
                                 this->client_streams_.emplace(stream);
                                 std::cout << "Received TransactionUpdates RPC request" << std::endl;
                             },
                             [this](void* stream) {
                                 this->client_streams_.erase(
                                     static_cast<net::ServerToClientStream<proto::HelloTransaction>*>(stream));
                                 std::cout << "Removed TransactionUpdates RPC request" << std::endl;
                             });
    }

    void run() { server_.run(); }

private:
    net::AsyncServer<proto::Greeter> server_;
    std::vector<proto::HelloTransaction> transactions_;
    std::unordered_set<net::ServerToClientStream<proto::HelloTransaction>*> client_streams_;

    grpc::Status say_hello(const proto::HelloRequest& request, proto::HelloResponse* response) {
        std::cout << "Received SayHello RPC request" << std::endl;

        response->set_message("Hello, " + request.name() + "!");

        proto::HelloTransaction transaction;
        *transaction.mutable_request() = request;
        *transaction.mutable_response() = *response;

        transactions_.emplace_back(std::move(transaction));

        return grpc::Status::OK;
    }

    void get_all_transactions(const google::protobuf::Empty& /*request*/,
                              net::ServerToClientStream<proto::HelloTransaction>* stream) {
        std::cout << "Received GetAllTransactions RPC request" << std::endl;

        for (const auto& transaction : transactions_) {
            stream->write(transaction);
        }

        stream->finish(grpc::Status::OK);
    }

    grpc::Status maybe_say_hello(const proto::HelloRequest& request, google::protobuf::Empty* /*response*/) {
        std::cout << "Received MaybeSayHello RPC request" << std::endl;

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
#endif

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

    std::cout << "Server running..." << std::endl;
    server->Wait();

    return 0;
}

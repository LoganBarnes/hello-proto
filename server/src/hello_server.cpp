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
                          proto::HelloReply* response) override {

        std::cout << "Recieved request: " << request->ShortDebugString() << std::endl;

        response->set_message("Hello, " + request->name() + "!");

        std::cout << "Sending response: " << response->ShortDebugString() << '\n' << std::endl;

        return grpc::Status::OK;
    }
};

} // namespace hello

int main() {

    constexpr const char* server_address = "0.0.0.0:9090";

    std::cout << "Server running..." << std::endl;

    hello::HelloService service;

    grpc::ServerBuilder builder;
    builder.RegisterService(&service);
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    server->Wait();

    return 0;
}

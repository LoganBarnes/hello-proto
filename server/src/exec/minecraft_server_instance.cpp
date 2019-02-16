#if 0
// project
#include "mcs/minecraft_server.hpp"

// system
#include <iostream>

int main(int argc, const char* argv[]) {

    std::string server_address = "0.0.0.0:9090";

    if (argc > 1) {
        server_address = argv[1];
    }

    mcs::MinecraftServer server(server_address);

    std::cout << "Server running at " << server_address << std::endl;
    server.run_blocking();

    return 0;
}
#else
// project
#include "net/async_server.hpp"

// generated
#include <minecraft/world.grpc.pb.h>

// third-party
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

// system
#include <iostream>

namespace {

grpc::Status say_hello(const minecraft::HelloRequest& request, minecraft::HelloReply* response) {
    response->set_message("Hello, " + request.name() + "!");
    //    std::cout << "Request:  " << request.ShortDebugString() << std::endl;
    //    std::cout << "Response: " << response->ShortDebugString() << std::endl;
    return grpc::Status::OK;
}

//void world_updates(const minecraft::ClientData& request) {
//    std::cout << "Client '" << request.name() << "' connected" << std::endl;
//}

} // namespace

int main() {
    net::AsyncServer<minecraft::World> minecraft_server(/*port=*/9090u);

    //    minecraft_server.register_server_stream_rpc(&minecraft::World::AsyncService::RequestWorldUpdates, &world_updates);
    minecraft_server.register_unary_rpc(&minecraft::World::AsyncService::RequestSayHello, &say_hello);

    minecraft_server.run();

    return 0;
}
#endif

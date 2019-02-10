#include "minecraft_server.hpp"

// third-party
#include <grpc++/server.h>
#include <grpc++/server_builder.h>

namespace mcs {

MinecraftServer::MinecraftServer(const std::string& host_address) {
    grpc::ServerBuilder builder;
    builder.RegisterService(&service_);
    builder.AddListeningPort(host_address, grpc::InsecureServerCredentials());

    server_ = builder.BuildAndStart();
}

void MinecraftServer::run_blocking() {
    server_->Wait();
}

} // namespace mcs

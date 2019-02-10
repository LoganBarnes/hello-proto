#pragma once

// project
#include "mcs/minecraft_world.hpp"

// generated
#include <minecraft/world.grpc.pb.h>

namespace mcs {

class WorldService : public minecraft::World::Service {
public:
    grpc::Status ModifyWorld(grpc::ServerContext* context,
                             const minecraft::WorldActionRequest* request,
                             minecraft::Errors* response) override;

    grpc::Status WorldUpdates(grpc::ServerContext* context,
                              const minecraft::ClientData* request,
                              grpc::ServerWriter<minecraft::WorldUpdate>* writer) override;

    grpc::Status SayHello(grpc::ServerContext* context,
                          const minecraft::HelloRequest* request,
                          minecraft::HelloReply* response) override;

private:
    // TODO: Make server as single threaded as possible
    // MinecraftWorld world_;

    std::mutex client_mutex_;
    std::unordered_map<const minecraft::ClientData*, grpc::ServerWriter<::minecraft::WorldUpdate>*> clients_;
};

} // namespace mcs

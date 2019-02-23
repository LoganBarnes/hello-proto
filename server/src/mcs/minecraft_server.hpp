#pragma once

// project
#include "mcs/minecraft_world.hpp"
#include "net/async_server.hpp"

// generated
#include <minecraft/world.grpc.pb.h>

// standard
#include <unordered_map>

namespace mcs {

class MinecraftServer {
public:
    explicit MinecraftServer(unsigned port);

    void run_blocking();

private:
    using UpdateStream = net::ServerToClientStream<minecraft::WorldUpdate>;
    using MetadataStream = net::ServerToClientStream<minecraft::Metadata>;

    net::AsyncServer<minecraft::World> server_;

    minecraft::Metadata metadata_;
    std::unordered_map<UpdateStream*, minecraft::ClientData> clients_;
    std::unordered_set<MetadataStream*> metadata_clients_;

    MinecraftWorld world_;

    void register_client(const minecraft::ClientData& client, UpdateStream* client_stream);
    void deregister_client(UpdateStream* client_stream);

    grpc::Status modify_world(const minecraft::WorldActionRequest& request, minecraft::Errors* errors);

    void send_metadata_to_clients();
    void send_update_to_clients(const minecraft::WorldUpdate& update);
};

} // namespace mcs

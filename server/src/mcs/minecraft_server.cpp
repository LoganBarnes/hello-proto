#include "minecraft_server.hpp"

// standard
#include <sstream>

namespace mcs {

namespace {
std::string to_string(net::ServerToClientStream<minecraft::WorldUpdate>* client_stream) {
    std::stringstream ss;
    ss << client_stream;
    return ss.str();
}
} // namespace

MinecraftServer::MinecraftServer(unsigned port) : server_(port) {
    metadata_.set_total_blocks(world_.blocks().size());

    // Let the clients modify the world
    server_.register_rpc(&minecraft::World::AsyncService::RequestModifyWorld,
                         [this](const minecraft::WorldActionRequest& request, minecraft::Errors* errors) {
                             return modify_world(request, errors);
                         });

    // Keep track of metadata streams requested by clients
    server_.register_rpc(&minecraft::World::AsyncService::RequestMetadataUpdates,
                         [this](const google::protobuf::Empty& /*ignored*/, MetadataStream* stream) {
                             metadata_clients_.emplace(stream);
                         },
                         [this](void* stream) { // client disconnected
                             metadata_clients_.erase(static_cast<MetadataStream*>(stream));
                         });

    // Keep track of update streams requested by clients
    server_.register_rpc(&minecraft::World::AsyncService::RequestWorldUpdates,
                         [this](const minecraft::ClientData& client, UpdateStream* client_stream) {
                             register_client(client, client_stream);
                         },
                         [this](void* client_stream) { // client disconnected
                             deregister_client(static_cast<UpdateStream*>(client_stream));
                         });
}

void MinecraftServer::run_blocking() {
    server_.run();
}

grpc::Status MinecraftServer::modify_world(const minecraft::WorldActionRequest& request, minecraft::Errors* errors) {
    minecraft::WorldUpdate update;

    switch (request.action_case()) {

    case minecraft::WorldActionRequest::kAddAdjacentBlock: {
        const minecraft::AddAdjacentBlockRequest& add_block_request = request.add_adjacent_block();
        *errors = world_.add_adjacent_block(add_block_request.existing_block(),
                                            add_block_request.adjacent_direction(),
                                            update.mutable_block_added()->mutable_block());

        if (errors->error_message().empty()) {
            metadata_.set_total_blocks(world_.blocks().size());

            send_update_to_clients(update);
            send_metadata_to_clients();
        }
    } break;

    case minecraft::WorldActionRequest::ACTION_NOT_SET:
        break;
    }

    return grpc::Status::OK;
}

void MinecraftServer::register_client(const minecraft::ClientData& client,
                                      MinecraftServer::UpdateStream* client_stream) {
    metadata_.mutable_clients()->insert({to_string(client_stream), client.name()});
    clients_.emplace(client_stream, client);

    // Send the current world state if the client requested it
    if (client.send_existing_state()) {
        minecraft::WorldUpdate update;
        minecraft::IVec3* position = update.mutable_block_added()->mutable_block()->mutable_position();

        for (const auto& block_pos : world_.blocks()) {
            position->set_x(std::get<0>(block_pos));
            position->set_y(std::get<1>(block_pos));
            position->set_z(std::get<2>(block_pos));

            client_stream->write(update);
        }
    }

    send_metadata_to_clients();
}

void MinecraftServer::deregister_client(MinecraftServer::UpdateStream* client_stream) {
    clients_.erase(client_stream);
    metadata_.mutable_clients()->erase(to_string(client_stream));

    send_metadata_to_clients();
}

void MinecraftServer::send_metadata_to_clients() {
    for (auto& client : metadata_clients_) {
        client->write(metadata_);
    }
}

void MinecraftServer::send_update_to_clients(const minecraft::WorldUpdate& update) {
    for (auto& client : clients_) {
        client.first->write(update);
    }
}

} // namespace mcs

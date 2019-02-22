#include "world_service.hpp"

namespace mcs {

grpc::Status WorldService::ModifyWorld(grpc::ServerContext* /*context*/,
                                       const minecraft::WorldActionRequest* /*request*/,
                                       minecraft::Errors* response) {

    response->set_error_message("Actions not handled yet");
    return grpc::Status::OK;
}

grpc::Status WorldService::WorldUpdates(grpc::ServerContext* /*context*/,
                                        const minecraft::ClientData* request,
                                        grpc::ServerWriter<::minecraft::WorldUpdate>* writer) {

    Client client{request, writer};
    Client* id = &client;

    // Add client to list of clients
    {
        std::lock_guard<std::mutex> lock(client_mutex_);
        clients_.emplace(id);
        std::cout << "(Client connected) " << request->ShortDebugString() << std::endl;
    }

    // Hold here in this thread and do nothing until the client is no longer connected.
    std::unique_lock<std::mutex> mutable_lock(client.mutex);
    client.client_blocker.wait(mutable_lock, [&client] { return client.client_is_gone; });

    // The client has disconnected. Remove it from the list of clients.
    {
        std::lock_guard<std::mutex> lock(client_mutex_);
        clients_.erase(id);
        std::cout << "(Client left) " << request->ShortDebugString() << std::endl;
    }

    return grpc::Status::OK;
}

} // namespace mcs

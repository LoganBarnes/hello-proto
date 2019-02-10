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

    std::lock_guard<std::mutex> lock(client_mutex_);
    clients_.emplace(request, writer);

    return grpc::Status::OK;
}

} // namespace mcs

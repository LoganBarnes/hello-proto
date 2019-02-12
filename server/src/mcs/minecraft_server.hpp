#pragma once

// project
#include "mcs/world_service.hpp"

// third-party
#include <grpc++/server.h>
#include <thread>

namespace mcs {

class MinecraftServer {
public:
    explicit MinecraftServer(const std::string& host_address);

    void run_blocking();

private:
    mcs::WorldService service_;
    std::unique_ptr<grpc::Server> server_;

    std::thread tmp_test_thread_;
};

} // namespace mcs

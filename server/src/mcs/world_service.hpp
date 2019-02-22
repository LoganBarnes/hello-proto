#pragma once

// project
#include "mcs/minecraft_world.hpp"

// generated
#include <condition_variable>
#include <minecraft/world.grpc.pb.h>
#include <thread>

namespace mcs {

class WorldService : public minecraft::World::Service {
public:
    grpc::Status ModifyWorld(grpc::ServerContext* context,
                             const minecraft::WorldActionRequest* request,
                             minecraft::Errors* response) override;

    grpc::Status WorldUpdates(grpc::ServerContext* context,
                              const minecraft::ClientData* request,
                              grpc::ServerWriter<minecraft::WorldUpdate>* writer) override;

    void loop() {
        int x = 0;
        int y = 0;
        int increment = -1;
        minecraft::WorldUpdate update;
        minecraft::IVec3* position = update.mutable_block_added()->mutable_block()->mutable_position();

        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (x > 10) {
                ++y;
                increment = -1;
            } else if (x < -10) {
                ++y;
                increment = 1;
            }

            position->set_x(x);
            position->set_y(y);
            x += increment;

            {
                std::lock_guard<std::mutex> lock(client_mutex_);
                std::cout << "Sending " << update.ShortDebugString() << std::endl;
                for (Client* client : clients_) {
                    if (!client->writer->Write(update)) {
                        // client stream is closed
                        client->client_is_gone = true;
                        client->client_blocker.notify_one();
                    }
                }
            }
        }
    }

private:
    // TODO: Make server as single threaded as possible
    // MinecraftWorld world_;

    struct Client {
        const minecraft::ClientData* client;
        grpc::ServerWriter<::minecraft::WorldUpdate>* writer;
        std::mutex mutex = {};
        std::condition_variable client_blocker = {};
        bool client_is_gone = false;
    };

    std::mutex client_mutex_;
    std::unordered_set<Client*> clients_;
};

} // namespace mcs

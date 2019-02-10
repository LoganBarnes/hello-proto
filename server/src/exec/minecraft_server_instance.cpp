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

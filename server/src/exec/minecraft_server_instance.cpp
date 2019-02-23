// project
#include <mcs/minecraft_server.hpp>

int main(int argc, const char* argv[]) {
    unsigned port = 9090u;

    if (argc > 1) {
        port = static_cast<unsigned>(std::stoul(argv[1]));
    }

    mcs::MinecraftServer server(/*port=*/port);
    server.run_blocking();
    return 0;
}

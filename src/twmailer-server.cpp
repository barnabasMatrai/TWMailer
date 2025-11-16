#include "TWMailerServer.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <mail-spool-directoryname>" << std::endl;
        return EXIT_FAILURE;
    }
    int port = std::stoi(argv[1]);
    std::string spoolDir = argv[2];

    TWMailerServer server(port, spoolDir);
    return server.run();
}

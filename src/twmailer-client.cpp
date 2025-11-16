#include "TWMailerClient.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string server_ip = argv[1];
    int port = std::stoi(argv[2]);

    TWMailerClient client(server_ip, port);
    client.run();
    return 0;
}

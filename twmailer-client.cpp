#include "TWMailerClient.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    try {
        std::string server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
        TWMailerClient client(server_ip);
        client.run();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

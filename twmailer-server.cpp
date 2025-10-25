#include "TWMailerServer.hpp"
#include <iostream>

int main() {
    try {
        TWMailerServer server;
        return server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

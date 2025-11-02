#include "TWMailerClient.hpp"
#include <iostream>
#include <cstdlib>
#include <string>
#include <regex>
#include <stdexcept>
#include <arpa/inet.h>

using std::cerr;
using std::endl;

using std::string;
using std::stoi;

using std::exception;

bool isValidIP(const string& ip) {
    sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1;
}

bool isValidPort(const string& portStr, int& portOut) {
    try {
        int port = stoi(portStr);
        if (port < 1 || port > 65535) return false;
        portOut = port;
        return true;
    } catch (...) {
        return false;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <ip> <port>" << endl;
        return EXIT_FAILURE;
    }

    std::string server_ip = argv[1];
    std::string portStr = argv[2];
    int port;

    if (!isValidIP(server_ip)) {
        cerr << "Error: Invalid IP address '" << server_ip << "'." << endl;
        return EXIT_FAILURE;
    }

    if (!isValidPort(portStr, port)) {
        cerr << "Error: Invalid port number '" << portStr << "'. Must be 1-65535." << endl;
        return EXIT_FAILURE;
    }

    try {
        TWMailerClient client(server_ip, port);
        client.run();
    } catch (const exception& e) {
        cerr << "Client error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

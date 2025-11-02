#include "TWMailerServer.hpp"
#include <iostream>
#include <cstdlib>
#include <string>
#include <filesystem>

using std::cout;
using std::cerr;
using std::endl;

using std::exception;

using std::string;
using std::stoi;

namespace fs = std::filesystem;

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
        cerr << "Usage: " << argv[0] << " <port> <mail-spool-directoryname>" << endl;
        return EXIT_FAILURE;
    }

    string portStr = argv[1];
    string spoolDir = argv[2];
    int port;

    if (!isValidPort(portStr, port)) {
        cerr << "Error: Invalid port number '" << portStr << "'. Must be between 1 and 65535." << endl;
        return EXIT_FAILURE;
    }

    // Validate or create mail spool directory
    try {
        if (!fs::exists(spoolDir)) {
            cout << "Mail spool directory '" << spoolDir << "' does not exist. Creating..." << endl;
            fs::create_directories(spoolDir);
        } else if (!fs::is_directory(spoolDir)) {
            cerr << "Error: '" << spoolDir << "' exists but is not a directory." << endl;
            return EXIT_FAILURE;
        }
    } catch (const fs::filesystem_error& e) {
        cerr << "Filesystem error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    try {
        TWMailerServer server(port, spoolDir);
        return server.run();
    } catch (const exception& e) {
        cerr << "Server error: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}

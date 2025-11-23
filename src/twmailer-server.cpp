#include "TWMailerServer.hpp"
#include <iostream>

using std::cerr;
using std::endl;
using std::stoi;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <port> <mail-spool-directoryname>" << endl;
        return EXIT_FAILURE;
    }
    int port = stoi(argv[1]);
    string spoolDir = argv[2];

    TWMailerServer server(port, spoolDir);
    return server.run();
}

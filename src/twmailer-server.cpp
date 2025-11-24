#include "TWMailerServer.hpp"
#include <iostream>

using std::cerr;
using std::endl;
using std::stoi;

// Entry point: starts the TWMailer server with the given port and spool directory
int main(int argc, char* argv[]) {

    // Expect exactly 2 arguments: port and mail spool directory
    if (argc != 3) {
        cerr << "Usage: " << argv[0]
             << " <port> <mail-spool-directoryname>" << endl;
        return EXIT_FAILURE;
    }

    int port = stoi(argv[1]);   // Convert port argument to integer
    string spoolDir = argv[2];  // Directory for storing user mail

    // Initialize and run the mail server
    TWMailerServer server(port, spoolDir);
    return server.run();        // run() returns program exit code
}

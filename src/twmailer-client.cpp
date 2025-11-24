#include "TWMailerClient.hpp"
#include <iostream>

using std::cerr;
using std::endl;
using std::stoi;

// Entry point: launches the TWMailer client and connects to the given server
int main(int argc, char* argv[]) {

    // Expect exactly 2 arguments: IP and port
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <ip> <port>" << endl;
        return EXIT_FAILURE;
    }

    string server_ip = argv[1];
    int port = stoi(argv[2]);  // Convert port argument to integer

    // Create client instance and start interactive mode
    TWMailerClient client(server_ip, port);
    client.run();

    return 0;
}

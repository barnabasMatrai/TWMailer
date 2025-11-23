#include "TWMailerClient.hpp"
#include <iostream>

using std::cerr;
using std::endl;
using std::stoi;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <ip> <port>" << endl;
        return EXIT_FAILURE;
    }

    string server_ip = argv[1];
    int port = stoi(argv[2]);

    TWMailerClient client(server_ip, port);
    client.run();
    return 0;
}

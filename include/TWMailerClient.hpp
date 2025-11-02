#ifndef TWMAILERCLIENT_HPP
#define TWMAILERCLIENT_HPP

#include "Utils.hpp"

#include <iostream>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/types.h>
#include <netdb.h>

#include <string>
#include <vector>
#include <sstream>
#include <string>
#include <netinet/in.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

using std::istringstream;
using std::ostringstream;

using std::runtime_error;
using std::invalid_argument;

constexpr int BUF = 1024;

class TWMailerClient {
public:
    explicit TWMailerClient(const string& server_ip, int port);
    ~TWMailerClient();

    void run();

private:
    int socket_fd = -1;
    sockaddr_in server_address{};
    int port;

    void createSocket();
    void initAddress(const string& server_ip);
    void connectToServer();
    bool receiveMessage();
    void sendMessage(const string& message);
    void closeConnection() noexcept;
};

#endif

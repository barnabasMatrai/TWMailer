#ifndef TWMAILERCLIENT_HPP
#define TWMAILERCLIENT_HPP

#include <string>
#include <termios.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <cstring>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::istringstream;
using std::ostringstream;
using std::runtime_error;

class TWMailerClient {
public:
    TWMailerClient(const string& server_ip, int port);
    ~TWMailerClient();

    void run();

private:
    string server_ip;
    int port;
    int socket_fd;

    bool create_socket();
    bool connect_to_server();

    bool recv_line_std(string& out);
    bool recv_line_and_print();
    void send_raw(const string& s);
};

#endif // TWMAILERCLIENT_HPP

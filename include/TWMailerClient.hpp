#ifndef TWMAILERCLIENT_HPP
#define TWMAILERCLIENT_HPP

#include <string>
#include <termios.h>

class TWMailerClient {
public:
    TWMailerClient(const std::string& server_ip, int port);
    ~TWMailerClient();

    void run();

private:
    std::string server_ip;
    int port;
    int socket_fd;

    bool createSocket();
    bool connectToServer();

    bool recv_line_std(std::string& out);
    bool recv_line_and_print();
    void send_raw(const std::string& s);
};

#endif // TWMAILERCLIENT_HPP

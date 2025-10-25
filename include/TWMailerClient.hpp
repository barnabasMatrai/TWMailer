#ifndef TWMAILERCLIENT_HPP
#define TWMAILERCLIENT_HPP

#include <string>
#include <netinet/in.h>

constexpr int BUF = 1024;
constexpr int PORT = 6543;

class TWMailerClient {
public:
    explicit TWMailerClient(const std::string& server_ip = "127.0.0.1");
    ~TWMailerClient();

    void run();

private:
    int socket_fd = -1;
    sockaddr_in server_address{};

    void createSocket();
    void initAddress(const std::string& server_ip);
    void connectToServer();
    bool receiveMessage();
    void sendMessage(const std::string& message);
    void closeConnection() noexcept;
};

#endif

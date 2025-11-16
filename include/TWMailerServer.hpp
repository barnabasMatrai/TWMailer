#ifndef TWMAILERSERVER_HPP
#define TWMAILERSERVER_HPP

#include <csignal>
#include <sstream>
#include <string>
#include <atomic>
#include <mutex>
#include "MailStore.hpp"
#include "AuthManager.hpp"

class TWMailerServer {
public:
    TWMailerServer(int port, const std::string& spoolDir);
    ~TWMailerServer();

    int run(); // blocks until shutdown

    static volatile sig_atomic_t abortRequested;

private:
    int port;
    std::string mailSpoolDir;
    MailStore store;
    AuthManager authManager;

    std::mutex store_mutex; // protect store operations

    int create_socket;

    void setupSignalHandler();
    void createSocket();
    void setSocketOptions();
    void bindSocket();
    void listenSocket();

    void clientThread(int clientfd, std::string client_ip);

    // command handlers (authenticated)
    bool readDotTerminatedBody(int sockfd, std::string& body);
    void handleSendAuthenticated(int clientfd, const std::string& sender);
    void handleListAuthenticated(int clientfd, const std::string& username);
    void handleReadAuthenticated(int clientfd, const std::string& username);
    void handleDelAuthenticated(int clientfd, const std::string& username);

    static void signalHandler(int sig);
};

#endif // TWMAILERSERVER_HPP

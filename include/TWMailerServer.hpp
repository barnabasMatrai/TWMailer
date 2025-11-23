#ifndef TWMAILERSERVER_HPP
#define TWMAILERSERVER_HPP

#include <csignal>
#include <sstream>
#include <string>
#include <atomic>
#include <mutex>
#include <iostream>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include "MailStore.hpp"
#include "AuthManager.hpp"

using std::string;
using std::thread;

class TWMailerServer {
public:
    TWMailerServer(int port, const string& spoolDir);
    ~TWMailerServer();

    int run(); // blocks until shutdown

    static volatile sig_atomic_t abortRequested;

private:
    int port;
    string mailSpoolDir;
    MailStore store;
    AuthManager authManager;

    mutex store_mutex; // protect store operations

    int create_socket;

    void setupSignalHandler();
    void createSocket();
    void setSocketOptions();
    void bindSocket();
    void listenSocket();

    void clientThread(int clientfd, string client_ip);

    // command handlers (authenticated)
    bool readDotTerminatedBody(int sockfd, string& body);
    void handleSendAuthenticated(int clientfd, const string& sender);
    void handleListAuthenticated(int clientfd, const string& username);
    void handleReadAuthenticated(int clientfd, const string& username);
    void handleDelAuthenticated(int clientfd, const string& username);

    static void signalHandler(int sig);
};

#endif // TWMAILERSERVER_HPP

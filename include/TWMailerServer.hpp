#ifndef TWMAILERSERVER_HPP
#define TWMAILERSERVER_HPP

#include "MailStore.hpp"
#include "Utils.hpp"

#include <iostream>
#include <string>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sstream>
#include <vector>

using std::cout;
using std::cerr;
using std::endl;

using std::string;
using std::stoul;
using std::vector;

using std::memset;
using std::ostringstream;

class TWMailerServer {
public:
    // Constructor & destructor
    TWMailerServer(int port, const string& spoolDir);
    ~TWMailerServer();

    // Run main accept loop
    int run();

    // Signal handler for graceful shutdown
    static void signalHandler(int sig);

private:
    // Class-level shared sockets (for signal handling)
    static volatile sig_atomic_t abortRequested;
    static int create_socket;
    static int new_socket;

    // Instance data
    int port;
    string mailSpoolDir;
    MailStore store;

    // Setup helpers
    void setupSignalHandler();
    void createSocket();
    void setSocketOptions();
    void bindSocket();
    void listenSocket();

    // Client communication and protocol handling
    void* clientCommunication(void* data);
    bool readDotTerminatedBody(int sockfd, string& body);
    void handleSend(int clientfd);
    void handleList(int clientfd);
    void handleRead(int clientfd);
    void handleDel(int clientfd);
};

#endif

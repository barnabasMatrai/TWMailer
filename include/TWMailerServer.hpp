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

    void setup_signal_handler();
    void create_server_socket();
    void set_socket_options();
    void bind_socket();
    void listen_socket();

    void client_thread(int clientfd, string client_ip);

    // command handlers (authenticated)
    bool read_dot_terminated_body(int sockfd, string& body);
    void handle_send(int clientfd, const string& sender);
    void handle_list(int clientfd, const string& username);
    void handle_read(int clientfd, const string& username);
    void handle_delete(int clientfd, const string& username);

    static void signal_handler(int sig);
};

#endif // TWMAILERSERVER_HPP

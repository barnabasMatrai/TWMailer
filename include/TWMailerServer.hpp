#ifndef TWMAILERSERVER_HPP
#define TWMAILERSERVER_HPP

#include <netinet/in.h>

constexpr int BUF = 1024;
constexpr int PORT = 6543;

class TWMailerServer {
public:
    TWMailerServer();
    ~TWMailerServer();

    int run();

private:
    static inline int abortRequested = 0;
    static inline int create_socket = -1;
    static inline int new_socket = -1;

    static void signalHandler(int sig);
    static void* clientCommunication(void* data);

    void setupSignalHandler();
    void createSocket();
    void setSocketOptions();
    void bindSocket();
    void listenSocket();
};

#endif

#include "TWMailerServer.hpp"
#include <iostream>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

TWMailerServer::TWMailerServer() {
    setupSignalHandler();
    createSocket();
    setSocketOptions();
    bindSocket();
    listenSocket();
}

TWMailerServer::~TWMailerServer() {
    if (create_socket != -1) {
        if (shutdown(create_socket, SHUT_RDWR) == -1) {
            perror("shutdown create_socket");
        }
        if (close(create_socket) == -1) {
            perror("close create_socket");
        }
        create_socket = -1;
    }
}

void TWMailerServer::setupSignalHandler() {
    if (signal(SIGINT, TWMailerServer::signalHandler) == SIG_ERR) {
        perror("signal can not be registered");
        exit(EXIT_FAILURE);
    }
}

void TWMailerServer::createSocket() {
    create_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (create_socket == -1) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }
}

void TWMailerServer::setSocketOptions() {
    int reuseValue = 1;
    if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEADDR, &reuseValue, sizeof(reuseValue)) == -1) {
        perror("set socket options - reuseAddr");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEPORT, &reuseValue, sizeof(reuseValue)) == -1) {
        perror("set socket options - reusePort");
        exit(EXIT_FAILURE);
    }
}

void TWMailerServer::bindSocket() {
    sockaddr_in address{};
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(create_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }
}

void TWMailerServer::listenSocket() {
    if (listen(create_socket, 5) == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }
}

int TWMailerServer::run() {
    sockaddr_in cliaddress{};
    socklen_t addrlen = sizeof(cliaddress);

    while (!abortRequested) {
        std::cout << "Waiting for connections..." << std::endl;

        new_socket = accept(create_socket, reinterpret_cast<sockaddr*>(&cliaddress), &addrlen);
        if (new_socket == -1) {
            if (abortRequested) {
                perror("accept error after aborted");
            } else {
                perror("accept error");
            }
            break;
        }

        std::cout << "Client connected from " << inet_ntoa(cliaddress.sin_addr)
                  << ":" << ntohs(cliaddress.sin_port) << "..." << std::endl;

        clientCommunication(&new_socket);
        new_socket = -1;
    }

    return EXIT_SUCCESS;
}

void* TWMailerServer::clientCommunication(void* data) {
    char buffer[BUF];
    int size;
    int* current_socket = static_cast<int*>(data);

    std::strcpy(buffer, "Welcome to myserver!\r\nPlease enter your commands...\r\n");
    if (send(*current_socket, buffer, std::strlen(buffer), 0) == -1) {
        perror("send failed");
        return nullptr;
    }

    do {
        size = recv(*current_socket, buffer, BUF - 1, 0);
        if (size == -1) {
            if (abortRequested) {
                perror("recv error after aborted");
            } else {
                perror("recv error");
            }
            break;
        }

        if (size == 0) {
            std::cout << "Client closed remote socket" << std::endl;
            break;
        }

        if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n') {
            size -= 2;
        } else if (buffer[size - 1] == '\n') {
            --size;
        }

        buffer[size] = '\0';
        std::cout << "Message received: " << buffer << std::endl;

        if (send(*current_socket, "OK", 3, 0) == -1) {
            perror("send answer failed");
            return nullptr;
        }

    } while (std::strcmp(buffer, "quit") != 0 && !abortRequested);

    if (*current_socket != -1) {
        if (shutdown(*current_socket, SHUT_RDWR) == -1) {
            perror("shutdown new_socket");
        }
        if (close(*current_socket) == -1) {
            perror("close new_socket");
        }
        *current_socket = -1;
    }

    return nullptr;
}

void TWMailerServer::signalHandler(int sig) {
    if (sig == SIGINT) {
        std::cout << "abort Requested... " << std::endl;
        abortRequested = 1;

        if (new_socket != -1) {
            if (shutdown(new_socket, SHUT_RDWR) == -1) {
                perror("shutdown new_socket");
            }
            if (close(new_socket) == -1) {
                perror("close new_socket");
            }
            new_socket = -1;
        }

        if (create_socket != -1) {
            if (shutdown(create_socket, SHUT_RDWR) == -1) {
                perror("shutdown create_socket");
            }
            if (close(create_socket) == -1) {
                perror("close create_socket");
            }
            create_socket = -1;
        }
    } else {
        exit(sig);
    }
}

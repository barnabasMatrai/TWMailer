#include "TWMailerClient.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

TWMailerClient::TWMailerClient(const std::string& server_ip) {
    createSocket();
    initAddress(server_ip);
    connectToServer();
}

TWMailerClient::~TWMailerClient() {
    closeConnection();
}

void TWMailerClient::createSocket() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        throw std::runtime_error("Socket creation failed");
    }
}

void TWMailerClient::initAddress(const std::string& server_ip) {
    std::memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_aton(server_ip.c_str(), &server_address.sin_addr) == 0) {
        throw std::invalid_argument("Invalid IP address");
    }
}

void TWMailerClient::connectToServer() {
    if (connect(socket_fd, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) == -1) {
        throw std::runtime_error("Connection failed: no server available");
    }
    std::cout << "Connection with server (" << inet_ntoa(server_address.sin_addr) << ") established\n";
}

bool TWMailerClient::receiveMessage() {
    char buffer[BUF];
    ssize_t size = recv(socket_fd, buffer, BUF - 1, 0);

    if (size == -1) {
        perror("recv error");
        return false;
    } else if (size == 0) {
        std::cout << "Server closed remote socket\n";
        return false;
    } else {
        buffer[size] = '\0';
        std::cout << "<< " << buffer << std::endl;
        if (std::strcmp(buffer, "OK") != 0 && std::strcmp(buffer, "quit") != 0) {
            std::cerr << "<< Server error occurred, abort\n";
            return false;
        }
    }
    return true;
}

void TWMailerClient::sendMessage(const std::string& message) {
    if (send(socket_fd, message.c_str(), message.size() + 1, 0) == -1) {
        perror("send error");
        throw std::runtime_error("Failed to send message");
    }
}

void TWMailerClient::closeConnection() noexcept {
    if (socket_fd != -1) {
        if (shutdown(socket_fd, SHUT_RDWR) == -1) {
            perror("shutdown socket_fd");
        }
        if (close(socket_fd) == -1) {
            perror("close socket_fd");
        }
        socket_fd = -1;
    }
}

void TWMailerClient::run() {
    receiveMessage();

    std::string input;
    bool isQuit = false;

    while (!isQuit) {
        std::cout << ">> ";
        if (!std::getline(std::cin, input)) break;

        if (input == "quit") {
            isQuit = true;
        }

        sendMessage(input);
        if (!receiveMessage()) break;
    }
}

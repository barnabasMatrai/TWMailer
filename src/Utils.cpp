#include "Utils.hpp"
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cctype>

ssize_t send_all(int socketFd, const string& data) {
    size_t totalSent = 0;
    const char* bufferPtr = data.data();
    size_t bytesRemaining = data.size();

    while (bytesRemaining > 0) {
        ssize_t bytesSent = send(socketFd, bufferPtr + totalSent, bytesRemaining, 0);

        if (bytesSent <= 0) {
            if (bytesSent < 0 && errno == EINTR) {
                // Interrupted by signal, retry
                continue;
            }
            // Any other error or peer closed connection
            return -1;
        }

        totalSent += static_cast<size_t>(bytesSent);
        bytesRemaining -= static_cast<size_t>(bytesSent);
    }

    return static_cast<ssize_t>(totalSent);
}


bool recv_line(int socketFd, string& line) {
    line.clear();
    char currentChar = 0;

    while (true) {
        ssize_t bytesReceived = recv(socketFd, &currentChar, 1, 0);

        if (bytesReceived == 0) {
            // Connection closed
            return false;
        }

        if (bytesReceived < 0) {
            if (errno == EINTR) {
                // Interrupted system call; retry
                continue;
            }
            // Other error
            return false;
        }

        if (currentChar == '\n') {
            // End of line reached
            break;
        }

        line.push_back(currentChar);
    }

    // Remove trailing carriage return (for CRLF line endings)
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    return true;
}


string trim_newline(const string &input) {
    string trimmed = input;
    
    while (!trimmed.empty() && (trimmed.back() == '\n' || trimmed.back() == '\r')) {
        trimmed.pop_back();
    }

    return trimmed;
}

bool valid_username(const string &username) {
    if (username.empty() || username.size() > 8) {
        return false;
    }

    for (char character : username) {
        unsigned char unsignedChar = (unsigned char)character;
        if (!(islower(unsignedChar) || isdigit(unsignedChar))) {
            return false;
        }
    }
    return true;
}

bool valid_subject(const string &subject) {
    // max 80 chars
    return subject.size() <= 80;
}

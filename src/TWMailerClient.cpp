#include "TWMailerClient.hpp"
#include "Utils.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <cstring>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

TWMailerClient::TWMailerClient(const string& server_ip_, int port_)
    : server_ip(server_ip_), port(port_), socket_fd(-1) {
}

TWMailerClient::~TWMailerClient() {
    if (socket_fd != -1) {
        shutdown(socket_fd, SHUT_RDWR);
        close(socket_fd);
        socket_fd = -1;
    }
}

bool TWMailerClient::createSocket() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket");
        return false;
    }
    return true;
}

bool TWMailerClient::connectToServer() {
    if (!createSocket()) return false;
    sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr) != 1) {
        cerr << "Invalid IP address" << endl;
        return false;
    }
    if (connect(socket_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("connect");
        return false;
    }
    return true;
}

bool TWMailerClient::recv_line_std(std::string& out) {
    return recv_line(socket_fd, out);
}

bool TWMailerClient::recv_line_and_print() {
    std::string line;
    if (!recv_line_std(line)) return false;
    cout << line;
    return true;
}

void TWMailerClient::send_raw(const std::string& s) {
    if (send_all(socket_fd, s) < 0) {
        throw std::runtime_error("send failed");
    }
}

void TWMailerClient::run() {
    if (!connectToServer()) {
        cerr << "Connection failed" << endl;
        return;
    }

    cout << "Connected. You must LOGIN first." << endl;
    string cmd;
    bool logged_in = false;
    string session_user;

    // optionally read greeting
    std::string greeting;
    if (recv_line_std(greeting)) {
        cout << "<< " << trim_newline(greeting) << endl;
    }

    while (true) {
        cout << "> ";
        if (!std::getline(std::cin, cmd)) break;
        if (cmd.empty()) continue;

        std::istringstream iss(cmd);
        string w; iss >> w;
        for (auto &c : w) c = std::toupper((unsigned char)c);

        if (w == "LOGIN") {
            string user, pass;
            cout << "Username: "; if (!std::getline(std::cin, user)) break;
            cout << "Password: "; if (!std::getline(std::cin, pass)) break;
            std::ostringstream out;
            out << "LOGIN\n" << user << "\n" << pass << "\n";
            try { send_raw(out.str()); } catch(...) { cerr << "Send failed\n"; break; }

            string resp;
            if (!recv_line_std(resp)) { cerr << "Server disconnected\n"; break; }
            resp = trim_newline(resp);
            if (resp == "OK") {
                logged_in = true;
                session_user = user;
                cout << "Login OK." << endl;
            } else {
                cout << "Login failed." << endl;
            }
        } else if (w == "QUIT") {
            try {
                send_raw(string("QUIT\n"));
            } catch(...) {}
            cout << "Disconnected" << endl;
            break;
        } else {
            if (!logged_in) {
                cout << "You must LOGIN first." << endl;
                continue;
            }

            if (w == "SEND") {
                string receiver, subject;
                cout << "Receiver: "; getline(std::cin, receiver);
                cout << "Subject: "; getline(std::cin, subject);
                if (!valid_username(receiver) || !valid_subject(subject)) {
                    cout << "Invalid receiver/subject" << endl;
                    continue;
                }
                std::ostringstream out;
                out << "SEND\n" << receiver << "\n" << subject << "\n";
                try { send_raw(out.str()); } catch(...) { cerr << "Send failed\n"; break; }

                cout << "Enter message body. End with single dot on a line:" << endl;
                while (true) {
                    string line;
                    if (!getline(std::cin, line)) break;
                    if (line == ".") break;
                    string sendline = line + "\n";
                    if (send_all(socket_fd, sendline) < 0) { cerr << "Send failed\n"; break; }
                }
                if (send_all(socket_fd, ".\n") < 0) { cerr << "Send failed\n"; break; }

                string resp;
                if (!recv_line_std(resp)) { cerr << "Server disconnected\n"; break; }
                cout << "<< " << trim_newline(resp) << endl;

            } else if (w == "LIST") {
                try { send_raw(string("LIST\n")); } catch(...) { cerr << "Send failed\n"; break; }
                string line;
                if (!recv_line_std(line)) { cerr << "Server disconnected\n"; break; }
                line = trim_newline(line);
                cout << "Server: " << line << endl;
                int count = 0;
                try { count = std::stoi(line); } catch(...) { count = 0; }
                for (int i = 0; i < count; ++i) {
                    if (!recv_line_std(line)) { cerr << "Server disconnected\n"; break; }
                    cout << (i+1) << ": " << trim_newline(line) << endl;
                }
            } else if (w == "READ") {
                cout << "Message-Number: ";
                string num; getline(std::cin, num);
                std::ostringstream out;
                out << "READ\n" << num << "\n";
                try { send_raw(out.str()); } catch(...) { cerr << "Send failed\n"; break; }
                string line;
                if (!recv_line_std(line)) { cerr << "Server disconnected\n"; break; }
                if (trim_newline(line) != "OK") {
                    cout << "Server: ERR" << endl;
                    continue;
                }
                // read message header/body
                string sender, receiver, subject;
                if (!recv_line_std(sender) || !recv_line_std(receiver) || !recv_line_std(subject)) {
                    cerr << "Server disconnected\n"; break;
                }
                cout << "Sender: " << trim_newline(sender) << endl;
                cout << "Receiver: " << trim_newline(receiver) << endl;
                cout << "Subject: " << trim_newline(subject) << endl;
                cout << "Body:" << endl;
                while (true) {
                    if (!recv_line_std(line)) { cerr << "Server disconnected\n"; break; }
                    if (trim_newline(line) == ".") break;
                    cout << line;
                }
            } else if (w == "DEL") {
                cout << "Message-Number: ";
                string num; getline(std::cin, num);
                std::ostringstream out;
                out << "DEL\n" << num << "\n";
                try { send_raw(out.str()); } catch(...) { cerr << "Send failed\n"; break; }
                string line;
                if (!recv_line_std(line)) { cerr << "Server disconnected\n"; break; }
                cout << "<< " << trim_newline(line) << endl;
            } else {
                cout << "Unknown command" << endl;
            }
        }
    }
}

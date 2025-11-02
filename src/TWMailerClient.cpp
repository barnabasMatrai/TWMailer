#include "TWMailerClient.hpp"

TWMailerClient::TWMailerClient(const string& server_ip, int port) : port(port) {
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
        throw runtime_error("Socket creation failed");
    }
}

void TWMailerClient::initAddress(const string& server_ip) {
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if (inet_aton(server_ip.c_str(), &server_address.sin_addr) == 0) {
        throw invalid_argument("Invalid IP address");
    }
}

void TWMailerClient::connectToServer() {
    if (connect(socket_fd, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) == -1) {
        throw runtime_error("Connection failed: no server available");
    }
    cout << "Connection with server (" << inet_ntoa(server_address.sin_addr) << ") established" << endl;
}

bool TWMailerClient::receiveMessage() {
    char buffer[BUF];
    ssize_t size = recv(socket_fd, buffer, BUF - 1, 0);

    if (size == -1) {
        perror("recv error");
        return false;
    } else if (size == 0) {
        cout << "Server closed remote socket" << endl;
        return false;
    } else {
        buffer[size] = '\0';
        cout << "<< " << buffer << endl;
        if (strcmp(buffer, "OK") != 0 && strcmp(buffer, "quit") != 0) {
            cerr << "<< Server error occurred, abort" << endl;
            return false;
        }
    }
    return true;
}

void TWMailerClient::sendMessage(const string& message) {
    if (send(socket_fd, message.c_str(), message.size() + 1, 0) == -1) {
        perror("send error");
        throw runtime_error("Failed to send message");
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
    string cmd;
    cout << "Connected. Type commands (SEND, LIST, READ, DEL, QUIT)." << endl;
    while (true) {
        cout << "> ";
        if (!getline(cin, cmd)) {
            break;
        }

        if (cmd.empty()) {
            continue;
        }
        string ucmd = cmd;
        // uppercase the command word for recognition
        {
            istringstream is(ucmd);
            string w; is >> w;
            for (auto &c : w) c = toupper((unsigned char)c);
            if (w == "SEND") {
                // gather SEND fields
                string sender, receiver, subject;
                cout << "Sender: "; getline(cin, sender);
                cout << "Receiver: "; getline(cin, receiver);
                cout << "Subject: "; getline(cin, subject);
                if (!valid_username(sender) || !valid_username(receiver) || !valid_subject(subject)) {
                    cout << "Invalid sender/receiver/subject" << endl;
                    continue;
                }
                // send header
                ostringstream out;
                out << "SEND" << endl;
                out << sender << endl;
                out << receiver << endl;
                out << subject << endl;
                if (send_all(socket_fd, out.str()) < 0) {
                    cerr << "Send failed" << endl;
                    break;
                }
                cout << "Enter message body. End with single dot on a line:" << endl;
                while (true) {
                    string line;
                    if (!getline(cin, line)) {
                        break;
                    }

                    string sendline = line + "\n";
                    if (line == ".") {
                        // protocol requires just ".\n" to end— but we've already sent it; done.
                        break;
                    }

                    if (send_all(socket_fd, sendline) < 0) {
                        cerr << "Send failed" << endl;
                        break;
                    }
                }
                // finally send the dot terminator if not already sent
                if (send_all(socket_fd, ".\n") < 0) {
                    cerr << "Send failed" << endl;
                    break;
                }
                // read response line
                string resp;
                if (!recv_line(socket_fd, resp)) {
                    cout << "Server disconnected" << endl;
                    break;
                }
                cout << "Server: " << resp << endl;
            } else if (w == "LIST") {
                string user;
                cout << "Username: "; getline(cin, user);
                if (!valid_username(user)) {
                    cout << "Invalid username" << endl;
                    continue;
                }
                ostringstream out;
                out << "LIST" << endl;
                out << user  << endl;
                if (send_all(socket_fd, out.str()) < 0) {
                    cerr << "Send failed" << endl;
                    break;
                }
                // read count line
                string line;
                if (!recv_line(socket_fd, line)) {
                    cout << "Server disconnected" << endl;
                    break;
                }
                cout << "Server: " << line  << endl;
                int count = 0;
                try {
                    count = stoi(trim_newline(line));
                } catch(...) {
                    count = 0;
                }
                for (int i = 0; i < count; ++i) {
                    if (!recv_line(socket_fd, line)) {
                        cout << "Server disconnected" << endl;
                        break;
                    }
                    cout << (i+1) << ": " << line  << endl;
                }
            } else if (w == "READ") {
                string user; string num;
                cout << "Username: "; getline(cin, user);
                cout << "Message-Number: "; getline(cin, num);
                if (!valid_username(user)) {
                    cout << "Invalid username" << endl;
                    continue;
                }
                ostringstream out;
                out << "READ" << endl << user << endl << num << endl;
                if (send_all(socket_fd, out.str()) < 0) {
                    cerr << "Send failed" << endl;
                    break;
                }
                string line;
                if (!recv_line(socket_fd, line)) {
                    cout << "Server disconnected" << endl;
                    break;
                }
                if (trim_newline(line) != "OK") {
                    cout << "Server: ERR" << endl;
                    continue;
                }
                // Next lines are message per SEND format until ".\n"
                string sender, receiver, subject, body;
                if (!recv_line(socket_fd, sender)) {
                    cout << "Server disconnected" << endl;
                    break;
                }
                if (!recv_line(socket_fd, receiver)) {
                    cout << "Server disconnected" << endl;
                    break;
                }
                if (!recv_line(socket_fd, subject)) {
                    cout << "Server disconnected" << endl;
                    break;
                }
                cout << "Sender: " << sender << endl;
                cout << "Receiver: " << receiver << endl;
                cout << "Subject: " << subject << endl;
                cout << "Body:" << endl;
                while (true) {
                    if (!recv_line(socket_fd, line)) {
                        cout << "Server disconnected" << endl;
                        break;
                    }
                    if (line == ".") {
                        break;
                    }
                    cout << line << endl;
                }
            } else if (w == "DEL") {
                string user; string num;
                cout << "Username: "; getline(cin, user);
                cout << "Message-Number: "; getline(cin, num);
                if (!valid_username(user)) {
                    cout << "Invalid username" << endl;
                    continue;
                }
                ostringstream out;
                out << "DEL" << endl << user << endl << num << endl;
                if (send_all(socket_fd, out.str()) < 0) {
                    cerr << "Send failed" << endl;
                    break;
                }
                string line;
                if (!recv_line(socket_fd, line)) {
                    cout << "Server disconnected" << endl;
                    break;
                }
                cout << "Server: " << line << endl;
            } else if (w == "QUIT") {
                ostringstream out;
                out << "QUIT" << endl;
                send_all(socket_fd, out.str());
                cout << "Disconnected" << endl;
                break;
            } else {
                cout << "Unknown command" << endl;
            }
        }
    }
}

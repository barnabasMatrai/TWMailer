#include "TWMailerClient.hpp"
#include "Utils.hpp"

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

bool TWMailerClient::create_socket() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket");
        return false;
    }
    return true;
}

bool TWMailerClient::connect_to_server() {
    if (!create_socket()) return false;
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

bool TWMailerClient::recv_line_std(string& out) {
    return recv_line(socket_fd, out);
}

bool TWMailerClient::recv_line_and_print() {
    string line;
    if (!recv_line_std(line)) return false;
    cout << line;
    return true;
}

void TWMailerClient::send_raw(const string& s) {
    if (send_all(socket_fd, s) < 0) {
        throw runtime_error("send failed");
    }
}

void TWMailerClient::run() {
    if (!connect_to_server()) {
        cerr << "Connection failed" << endl;
        return;
    }

    cout << "Connected. You must LOGIN first." << endl;
    string cmd;
    bool logged_in = false;
    string session_user;

    string greeting;
    if (recv_line_std(greeting)) {
        cout << "<< " << trim_newline(greeting) << endl;
    }

    while (true) {
        cout << "> ";
        if (!getline(cin, cmd)) break;
        if (cmd.empty()) continue;

        istringstream iss(cmd);
        string input; iss >> input;
        for (auto &c : input) c = toupper((unsigned char)c);

        if (input == "LOGIN") {
            string user, pass;
            cout << "Username: "; if (!getline(cin, user)) break;
            termios oldt, newt;
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= ~ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);

            cout << "Password: "; if (!getline(cin, pass)) break;
            cout << endl;

            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            ostringstream out;
            out << "LOGIN" << endl << user << endl << pass << endl;
            try { send_raw(out.str()); } catch(...) { cerr << "Send failed" << endl; break; }

            string resp;
            if (!recv_line_std(resp)) { cerr << "Server disconnected" << endl; break; }
            resp = trim_newline(resp);
            if (resp == "OK") {
                logged_in = true;
                session_user = user;
                cout << "Login OK." << endl;
            } else {
                cout << "Login failed." << endl;
            }
        } else if (input == "QUIT") {
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

            if (input == "SEND") {
                string receiver, subject;
                cout << "Receiver: "; getline(cin, receiver);
                cout << "Subject: "; getline(cin, subject);
                if (!valid_username(receiver) || !valid_subject(subject)) {
                    cout << "Invalid receiver/subject" << endl;
                    continue;
                }
                ostringstream out;
                out << "SEND" << endl << receiver << endl << subject << endl;
                try { send_raw(out.str()); } catch(...) { cerr << "Send failed" << endl; break; }

                cout << "Enter message body. End with single dot on a line:" << endl;
                while (true) {
                    string line;
                    if (!getline(cin, line)) break;
                    if (line == ".") break;
                    string sendline = line + "\n";
                    if (send_all(socket_fd, sendline) < 0) { cerr << "Send failed" << endl; break; }
                }
                if (send_all(socket_fd, ".\n") < 0) { cerr << "Send failed" << endl; break; }

                string resp;
                if (!recv_line_std(resp)) { cerr << "Server disconnected" << endl; break; }
                cout << "<< " << trim_newline(resp) << endl;

            } else if (input == "LIST") {
                try { send_raw(string("LIST\n")); } catch(...) { cerr << "Send failed" << endl; break; }
                string line;
                if (!recv_line_std(line)) { cerr << "Server disconnected" << endl; break; }
                line = trim_newline(line);
                cout << "Server: " << line << endl;
                int count = 0;
                try { count = stoi(line); } catch(...) { count = 0; }
                for (int i = 0; i < count; ++i) {
                    if (!recv_line_std(line)) { cerr << "Server disconnected" << endl; break; }
                    cout << (i+1) << ": " << trim_newline(line) << endl;
                }
            } else if (input == "READ") {
                cout << "Message-Number: ";
                string num; getline(cin, num);
                ostringstream out;
                out << "READ" << endl << num << endl;
                try { send_raw(out.str()); } catch(...) { cerr << "Send failed" << endl; break; }
                string line;
                if (!recv_line_std(line)) { cerr << "Server disconnected" << endl; break; }
                if (trim_newline(line) != "OK") {
                    cout << "Server: ERR" << endl;
                    continue;
                }
                // read message header/body
                string sender, receiver, subject;
                if (!recv_line_std(sender) || !recv_line_std(receiver) || !recv_line_std(subject)) {
                    cerr << "Server disconnected" << endl; break;
                }
                cout << "Sender: " << trim_newline(sender) << endl;
                cout << "Receiver: " << trim_newline(receiver) << endl;
                cout << "Subject: " << trim_newline(subject) << endl;
                cout << "Body:" << endl;
                while (true) {
                    if (!recv_line_std(line)) { cerr << "Server disconnected" << endl; break; }
                    if (trim_newline(line) == ".") break;
                    cout << line;
                }
            } else if (input == "DEL") {
                cout << "Message-Number: ";
                string num; getline(cin, num);
                ostringstream out;
                out << "DEL" << endl << num << endl;
                try { send_raw(out.str()); } catch(...) { cerr << "Send failed" << endl; break; }
                string line;
                if (!recv_line_std(line)) { cerr << "Server disconnected" << endl; break; }
                cout << "<< " << trim_newline(line) << endl;
            } else {
                cout << "Unknown command" << endl;
            }
        }
    }
}

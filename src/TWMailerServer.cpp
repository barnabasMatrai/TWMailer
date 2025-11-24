#include "TWMailerServer.hpp"
#include "Utils.hpp"

// global abort flag, set by SIGINT handler
volatile sig_atomic_t TWMailerServer::abortRequested = 0;

// Constructor: initialize server, auth manager, message store, and TCP socket
TWMailerServer::TWMailerServer(int port_, const string& spoolDir)
    : port(port_), mailSpoolDir(spoolDir), store(spoolDir),
      authManager("ldap://ldap.technikum.wien.at", "dc=technikum-wien,dc=at", spoolDir + "/blacklist.db"),
      create_socket(-1)
{
    setup_signal_handler();
    create_server_socket();
    set_socket_options();
    bind_socket();
    listen_socket();

    // Ensure spool directories/files exist
    string err;
    if (!store.ensure_spool_ok(err)) {
        cerr << "Mail spool setup failed: " << err << endl;
        exit(EXIT_FAILURE);
    }
}

// Destructor: clean shutdown of the listening socket
TWMailerServer::~TWMailerServer() {
    if (create_socket != -1) {
        shutdown(create_socket, SHUT_RDWR);
        close(create_socket);
        create_socket = -1;
    }
}

// Installs SIGINT signal handler for graceful shutdown
void TWMailerServer::setup_signal_handler() {
    if (signal(SIGINT, TWMailerServer::signal_handler) == SIG_ERR) {
        perror("signal cannot be registered");
        exit(EXIT_FAILURE);
    }
}

// Creates the server's TCP listening socket
void TWMailerServer::create_server_socket() {
    create_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (create_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
}

// Configures socket options (SO_REUSEADDR)
void TWMailerServer::set_socket_options() {
    int reuseValue = 1;
    if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEADDR, &reuseValue, sizeof(reuseValue)) == -1) {
        perror("setsockopt SO_REUSEADDR");
        exit(EXIT_FAILURE);
    }
}

// Binds the listening socket to the given port on all interfaces
void TWMailerServer::bind_socket() {
    sockaddr_in address{};
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(create_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
}

// Configures socket to begin listening for incoming connections
void TWMailerServer::listen_socket() {
    if (listen(create_socket, 16) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

// Main server loop: accepts connections and launches worker threads
int TWMailerServer::run() {
    sockaddr_in cliaddress{};
    socklen_t addrlen = sizeof(cliaddress);

    while (!abortRequested) {
        cout << "Waiting for connections..." << endl;

        // Accept connection from client
        int new_socket = accept(create_socket, reinterpret_cast<sockaddr*>(&cliaddress), &addrlen);
        if (new_socket == -1) {
            if (abortRequested) perror("accept after abort");
            else perror("accept");
            continue;
        }

        string client_ip = inet_ntoa(cliaddress.sin_addr);
        int client_port = ntohs(cliaddress.sin_port);

        cout << "Client connected from " << client_ip << ":" << client_port << endl;

        // Each client handled in separate detached thread
        thread worker(&TWMailerServer::client_thread, this, new_socket, client_ip);
        worker.detach();
    }

    return EXIT_SUCCESS;
}

// Handles a single client connection lifecycle
void TWMailerServer::client_thread(int clientfd, string client_ip) {
    bool authenticated = false;
    string session_user;

    // Send greeting
    send_all(clientfd, "OK\n");

    // Immediately reject blacklisted IPs
    if (authManager.is_ip_blacklisted(client_ip)) {
        send_all(clientfd, "ERR\n");
        close(clientfd);
        return;
    }

    string line;
    while (!abortRequested) {
        if (!recv_line(clientfd, line)) break;
        string cmd = trim_newline(line);

        // LOGIN command handling
        if (cmd == "LOGIN") {
            string username, password;
            if (!recv_line(clientfd, username) || !recv_line(clientfd, password)) {
                send_all(clientfd, "ERR\n");
                continue;
            }
            username = trim_newline(username);
            password = trim_newline(password);

            // Prevent login attempts from blacklisted clients
            if (authManager.is_ip_blacklisted(client_ip)) {
                send_all(clientfd, "ERR\n");
                continue;
            }

            string err;
            // Authenticate user via LDAP and blacklist manager
            if (authManager.authenticate(username, password, client_ip, err)) {
                authenticated = true;
                session_user = username;
                send_all(clientfd, "OK\n");
            } else {
                send_all(clientfd, "ERR\n");
            }
        }
        // QUIT command ends session
        else if (cmd == "QUIT") {
            send_all(clientfd, "OK\n");
            break;
        }
        // All other commands require successful login
        else {
            if (!authenticated) {
                send_all(clientfd, "ERR\n");
                continue;
            }

            if (cmd == "SEND") {
                handle_send(clientfd, session_user);
            } else if (cmd == "LIST") {
                handle_list(clientfd, session_user);
            } else if (cmd == "READ") {
                handle_read(clientfd, session_user);
            } else if (cmd == "DEL") {
                handle_delete(clientfd, session_user);
            } else {
                send_all(clientfd, "ERR\n");
            }
        }
    }

    // Cleanup and log disconnection
    shutdown(clientfd, SHUT_RDWR);
    close(clientfd);
    cout << "Client (" << client_ip << ") disconnected." << endl;
}

// Reads message body until single '.' line; stores into body string
bool TWMailerServer::read_dot_terminated_body(int sockfd, string& body) {
    body.clear();
    string line;
    bool first = true;

    while (true) {
        if (!recv_line(sockfd, line)) {
            return false;
        }

        // End of message body marker
        if (line == ".\n" || line == ".\r\n") {
            break;
        }

        string t = trim_newline(line);
        if (!first) body += "\n";
        body += t;
        first = false;
    }
    return true;
}

// Handles SEND command: validates fields, reads body, stores message
void TWMailerServer::handle_send(int clientfd, const string& sender) {
    string receiver, subject, body;

    if (!recv_line(clientfd, receiver) || !recv_line(clientfd, subject)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    receiver = trim_newline(receiver);
    subject  = trim_newline(subject);

    // Validate basic syntax
    if (!valid_username(receiver) || !valid_subject(subject)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    // Read DOT-terminated message body
    if (!read_dot_terminated_body(clientfd, body)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    // Construct message struct
    Message msg{ "", sender, receiver, subject, body };

    // Store under lock
    string err;
    {
        lock_guard<mutex> lg(store_mutex);
        if (!store.store_message(msg, err)) {
            cerr << "Store error: " << err << endl;
            send_all(clientfd, "ERR\n");
            return;
        }
    }

    send_all(clientfd, "OK\n");
}

// Handles LIST command: sends number of messages + subject lines
void TWMailerServer::handle_list(int clientfd, const string& username) {
    vector<string> subjects;

    {
        lock_guard<mutex> lg(store_mutex);
        if (!store.list_subjects(username, subjects)) {
            send_all(clientfd, "0\n");
            return;
        }
    }

    ostringstream oss;
    oss << subjects.size() << "\n";
    for (auto& s : subjects) oss << s << "\n";
    send_all(clientfd, oss.str());
}

// Handles READ command: retrieves full message and sends formatted output
void TWMailerServer::handle_read(int clientfd, const string& username) {
    string numStr;
    if (!recv_line(clientfd, numStr)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    numStr = trim_newline(numStr);

    size_t num = 0;
    try {
        num = stoul(numStr);    // convert message index
    } catch (...) {
        send_all(clientfd, "ERR\n");
        return;
    }

    optional<Message> msgOpt;
    {
        lock_guard<mutex> lg(store_mutex);
        msgOpt = store.read_message(username, num);
    }

    if (!msgOpt) {
        send_all(clientfd, "ERR\n");
        return;
    }

    auto& msg = *msgOpt;

    // Build protocol-compliant message response
    ostringstream oss;
    oss << "OK\n";
    oss << msg.sender   << "\n";
    oss << msg.receiver << "\n";
    oss << msg.subject  << "\n";

    if (!msg.body.empty()) {
        oss << msg.body;
        if (msg.body.back() != '\n')
            oss << "\n";
    }
    oss << ".\n";

    send_all(clientfd, oss.str());
}

// Handles DEL command: deletes message by index
void TWMailerServer::handle_delete(int clientfd, const string& username) {
    string numStr;
    if (!recv_line(clientfd, numStr)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    numStr = trim_newline(numStr);

    size_t num = 0;
    try {
        num = stoul(numStr);
    } catch (...) {
        send_all(clientfd, "ERR\n");
        return;
    }

    string err;
    {
        lock_guard<mutex> lg(store_mutex);
        if (!store.delete_message(username, num, err)) {
            send_all(clientfd, "ERR\n");
            return;
        }
    }

    send_all(clientfd, "OK\n");
}

// SIGINT handler: triggers server shutdown
void TWMailerServer::signal_handler(int sig) {
    if (sig == SIGINT) {
        cout << "\nAbort requested..." << endl;
        abortRequested = 1;
    } else {
        exit(sig);
    }
}

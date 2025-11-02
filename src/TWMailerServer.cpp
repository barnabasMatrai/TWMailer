#include "TWMailerServer.hpp"

volatile sig_atomic_t TWMailerServer::abortRequested = 0;
int TWMailerServer::create_socket = -1;
int TWMailerServer::new_socket = -1;

TWMailerServer::TWMailerServer(int port, const string& spoolDir)
    : port(port), mailSpoolDir(spoolDir), store(spoolDir) {
    setupSignalHandler();
    createSocket();
    setSocketOptions();
    bindSocket();
    listenSocket();

    string err;
    if (!store.ensure_spool_ok(err)) {
        cerr << "Mail spool setup failed: " << err << endl;
        exit(EXIT_FAILURE);
    }
}

TWMailerServer::~TWMailerServer() {
    if (create_socket != -1) {
        shutdown(create_socket, SHUT_RDWR);
        close(create_socket);
        create_socket = -1;
    }
}

void TWMailerServer::setupSignalHandler() {
    if (signal(SIGINT, TWMailerServer::signalHandler) == SIG_ERR) {
        perror("signal cannot be registered");
        exit(EXIT_FAILURE);
    }
}

void TWMailerServer::createSocket() {
    create_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (create_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
}

void TWMailerServer::setSocketOptions() {
    int reuseValue = 1;
    if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEADDR, &reuseValue, sizeof(reuseValue)) == -1) {
        perror("setsockopt SO_REUSEADDR");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEPORT, &reuseValue, sizeof(reuseValue)) == -1) {
        perror("setsockopt SO_REUSEPORT");
        exit(EXIT_FAILURE);
    }
}

void TWMailerServer::bindSocket() {
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

void TWMailerServer::listenSocket() {
    if (listen(create_socket, 5) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

int TWMailerServer::run() {
    sockaddr_in cliaddress{};
    socklen_t addrlen = sizeof(cliaddress);

    while (!abortRequested) {
        cout << "Waiting for connections..." << endl;

        new_socket = accept(create_socket, reinterpret_cast<sockaddr*>(&cliaddress), &addrlen);
        if (new_socket == -1) {
            if (abortRequested) perror("accept after abort");
            else perror("accept");
            continue;
        }

        cout << "Client connected from "
                  << inet_ntoa(cliaddress.sin_addr)
                  << ":" << ntohs(cliaddress.sin_port)
                  << endl;

        clientCommunication(&new_socket);

        if (new_socket != -1) {
            shutdown(new_socket, SHUT_RDWR);
            close(new_socket);
            new_socket = -1;
        }

        cout << "Client disconnected." << endl;
    }

    return EXIT_SUCCESS;
}

// -----------------------------
// CLIENT COMMUNICATION
// -----------------------------
void* TWMailerServer::clientCommunication(void* data) {
    int* sock = static_cast<int*>(data);
    int clientfd = *sock;
    string line;

    while (!abortRequested) {
        if (!recv_line(clientfd, line)) break;
        string cmd = trim_newline(line);

        if (cmd == "SEND") {
            handleSend(clientfd);
        } else if (cmd == "LIST") {
            handleList(clientfd);
        } else if (cmd == "READ") {
            handleRead(clientfd);
        } else if (cmd == "DEL") {
            handleDel(clientfd);
        } else if (cmd == "QUIT") {
            break;
        } else {
            send_all(clientfd, "ERR\n");
        }
    }

    return nullptr;
}

// -----------------------------
// COMMAND HANDLERS
// -----------------------------
bool TWMailerServer::readDotTerminatedBody(int sockfd, string& body) {
    body.clear();
    string line;
    bool first = true;
    
    while (true) {
        if (!recv_line(sockfd, line)) {
            return false;
        }
        if (line == ".") {
            break;
        }
        if (!first) {
            body += "\n";
        }
        body += line;
        first = false;
    }

    return true;
}

void TWMailerServer::handleSend(int clientfd) {
    string sender, receiver, subject, body;
    if (!recv_line(clientfd, sender) || !recv_line(clientfd, receiver) || !recv_line(clientfd, subject)) {
        send_all(clientfd, "ERR\n");
        return;
    }
    sender = trim_newline(sender);
    receiver = trim_newline(receiver);
    subject = trim_newline(subject);

    if (!valid_username(sender) || !valid_username(receiver) || !valid_subject(subject)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    if (!readDotTerminatedBody(clientfd, body)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    Message msg{ "", sender, receiver, subject, body };
    string err;
    if (!store.store_message(msg, err)) {
        cerr << "Store error: " << err << endl;
        send_all(clientfd, "ERR\n");
        return;
    }

    send_all(clientfd, "OK\n");
}

void TWMailerServer::handleList(int clientfd) {
    string user;
    if (!recv_line(clientfd, user)) {
        send_all(clientfd, "ERR\n");
        return;
    }
    user = trim_newline(user);

    if (!valid_username(user)) {
        send_all(clientfd, "0\n");
        return;
    }

    vector<string> subjects;
    if (!store.list_subjects(user, subjects)) {
        send_all(clientfd, "0\n");
        return;
    }

    ostringstream oss;
    oss << subjects.size() << endl;
    for (auto& s : subjects) oss << s << endl;
    send_all(clientfd, oss.str());
}

void TWMailerServer::handleRead(int clientfd) {
    string user, numStr;
    if (!recv_line(clientfd, user) || !recv_line(clientfd, numStr)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    user = trim_newline(user);
    numStr = trim_newline(numStr);
    if (!valid_username(user)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    size_t num = 0;
    try {
        num = stoul(numStr);
    } catch (...) {
        send_all(clientfd, "ERR\n");
        return;
    }

    auto msgOpt = store.read_message(user, num);
    if (!msgOpt) {
        send_all(clientfd, "ERR\n");
        return;
    }

    auto& msg = *msgOpt;
    ostringstream oss;
    oss << "OK" << endl
        << msg.sender << endl
        << msg.receiver << endl
        << msg.subject << endl;

    if (!msg.body.empty()) {
        oss << msg.body;
        if (msg.body.back() != '\n') {
            oss << endl;
        }
    }
    oss << "." << endl;

    send_all(clientfd, oss.str());
}

void TWMailerServer::handleDel(int clientfd) {
    string user, numStr;
    if (!recv_line(clientfd, user) || !recv_line(clientfd, numStr)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    user = trim_newline(user);
    numStr = trim_newline(numStr);
    if (!valid_username(user)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    size_t num = 0;
    try {
        num = stoul(numStr);
    } catch (...) {
        send_all(clientfd, "ERR\n");
        return;
    }

    string err;
    if (!store.delete_message(user, num, err)) {
        send_all(clientfd, "ERR\n");
        return;
    }

    send_all(clientfd, "OK\n");
}

// -----------------------------
// SIGNAL HANDLER
// -----------------------------
void TWMailerServer::signalHandler(int sig) {
    if (sig == SIGINT) {
        cout << "\nAbort requested..." << endl;
        abortRequested = 1;

        if (new_socket != -1) {
            shutdown(new_socket, SHUT_RDWR);
            close(new_socket);
            new_socket = -1;
        }
        if (create_socket != -1) {
            shutdown(create_socket, SHUT_RDWR);
            close(create_socket);
            create_socket = -1;
        }
    } else {
        exit(sig);
    }
}

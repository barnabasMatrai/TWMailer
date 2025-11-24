#include "Utils.hpp"

// Remove trailing newline or carriage return characters
string trim_newline(const string& s) {
    string out = s;
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
        out.pop_back();
    return out;
}

// Check if username contains only valid characters and length
bool valid_username(const string& u) {
    if (u.empty() || u.size() > 64) return false;
    for (char c : u) {
        if (!(isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.'))
            return false;
    }
    return true;
}

// Check if subject length is acceptable
bool valid_subject(const string& s) {
    if (s.size() > 256) return false;
    return true;
}

// Send entire string reliably over socket
int send_all(int sockfd, const string& data) {
    const char* ptr = data.c_str();
    size_t left = data.size();
    while (left > 0) {
        ssize_t n = send(sockfd, ptr, left, 0);
        if (n <= 0) {
            if (n == -1 && errno == EINTR) continue; // retry on EINTR
            perror("send");
            return -1;
        }
        ptr += n;
        left -= n;
    }
    return 0;
}

// Read one line (up to '\n') from socket
bool recv_line(int sockfd, string& out) {
    out.clear();
    char buf;
    bool got_any = false;
    while (true) {
        ssize_t n = recv(sockfd, &buf, 1, 0);
        if (n == -1) {
            if (errno == EINTR) continue; // retry on interrupt
            perror("recv");
            return false;
        } else if (n == 0) {
            return false; // client closed connection
        } else {
            got_any = true;
            out.push_back(buf);
            if (buf == '\n') break; // end of line
            if (out.size() > 16 * 1024) return false; // line too long
        }
    }
    return got_any;
}

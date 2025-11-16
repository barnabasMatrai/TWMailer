#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sys/socket.h>

static constexpr int BUF_SIZE = 4096;

std::string trim_newline(const std::string& s);
bool valid_username(const std::string& u);
bool valid_subject(const std::string& s);

// send_all: send whole buffer (returns 0 on success, -1 on error)
int send_all(int sockfd, const std::string& data);

// recv_line: read one line (terminated by '\n'), return false on error/closed
bool recv_line(int sockfd, std::string& out);

#endif // UTILS_HPP

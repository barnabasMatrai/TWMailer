#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sys/socket.h>
#include <algorithm>
#include <cctype>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>

using std::string;

static constexpr int BUF_SIZE = 4096;

string trim_newline(const string& s);
bool valid_username(const string& u);
bool valid_subject(const string& s);

// send_all: send whole buffer (returns 0 on success, -1 on error)
int send_all(int sockfd, const string& data);

// recv_line: read one line (terminated by '\n'), return false on error/closed
bool recv_line(int sockfd, string& out);

#endif // UTILS_HPP

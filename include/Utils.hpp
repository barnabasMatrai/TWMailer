#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sys/socket.h>

using std::string;
using std::islower;
using std::isdigit;

ssize_t send_all(int sockfd, const string &data);
bool recv_line(int sockfd, string &line); // reads until '\n', strips '\r' and '\n'
string trim_newline(const string &s);
bool valid_username(const string &u);
bool valid_subject(const string &s);

#endif // UTILS_HPP

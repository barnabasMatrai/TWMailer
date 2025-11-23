#ifndef AUTHMANAGER_HPP
#define AUTHMANAGER_HPP

#include <ldap.h>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;
using std::pair;
using std::string;
using std::mutex;
using std::map;
using std::lock_guard;
using clk = std::chrono::system_clock;
using std::chrono::system_clock;
using std::chrono::minutes;

class Blacklist;

class AuthManager {
public:
    AuthManager(const string& ldap_uri = "ldap://ldap.technikum.wien.at",
                const string& search_base = "dc=technikum-wien,dc=at",
                const string& blacklist_file = "blacklist.db");

    ~AuthManager();

    bool authenticate(const string& username, const string& password, const string& client_ip, string& err);
    bool is_ip_blacklisted(const string& ip);
    void persist_blacklist();

private:
    bool ldap_check_credentials(const string& username, const string& password, string& err);
    void register_failed_attempt(const string& username, const string& ip);
    void register_successful_login(const string& username, const string& ip);

    map<string, pair<int, system_clock::time_point>> attempts_;
    mutex attempts_mtx_;

    string ldap_uri_;
    string search_base_;
    Blacklist* blacklist_;
    string blacklist_file_;
};

#endif // AUTHMANAGER_HPP

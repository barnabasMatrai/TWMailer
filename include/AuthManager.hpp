#ifndef AUTHMANAGER_HPP
#define AUTHMANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>

class Blacklist;

class AuthManager {
public:
    AuthManager(const std::string& ldap_uri = "ldap://ldap.technikum.wien.at",
                const std::string& search_base = "dc=technikum-wien,dc=at",
                const std::string& blacklist_file = "blacklist.db");

    ~AuthManager();

    bool authenticate(const std::string& username, const std::string& password, const std::string& client_ip, std::string& err);
    bool is_ip_blacklisted(const std::string& ip);
    void persist_blacklist();

private:
    bool ldap_check_credentials(const std::string& username, const std::string& password, std::string& err);
    void register_failed_attempt(const std::string& username, const std::string& ip);
    void register_successful_login(const std::string& username, const std::string& ip);

    std::map<std::string, std::pair<int, std::chrono::system_clock::time_point>> attempts_;
    std::mutex attempts_mtx_;

    std::string ldap_uri_;
    std::string search_base_;
    Blacklist* blacklist_;
    std::string blacklist_file_;
};

#endif // AUTHMANAGER_HPP

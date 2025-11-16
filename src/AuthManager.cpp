#include "AuthManager.hpp"
#include "Blacklist.hpp"

#include <ldap.h>
#include <iostream>
#include <sstream>
#include <chrono>

using namespace std;
using clk = chrono::system_clock;

static string make_key(const string& u, const string& ip) {
    return u + "|" + ip;
}

AuthManager::AuthManager(const string& ldap_uri,
                         const string& search_base,
                         const string& blacklist_file)
    : ldap_uri_(ldap_uri),
      search_base_(search_base),
      blacklist_file_(blacklist_file)
{
    blacklist_ = new Blacklist(blacklist_file_);
    blacklist_->load();
}

AuthManager::~AuthManager() {
    persist_blacklist();
    delete blacklist_;
}

bool AuthManager::is_ip_blacklisted(const std::string& ip) {
    blacklist_->garbage_collect();
    return blacklist_->is_blocked(ip);
}

void AuthManager::persist_blacklist() {
    blacklist_->garbage_collect();
    blacklist_->save();
}

void AuthManager::register_failed_attempt(const std::string& username, const std::string& ip) {
    string key = make_key(username, ip);
    lock_guard<mutex> g(attempts_mtx_);
    auto& pair = attempts_[key];
    pair.first += 1;

    if (pair.first >= 3) {
        auto until = clk::now() + chrono::minutes(1);
        pair.second = until;
        blacklist_->add(ip, until);
        pair.first = 0;
        persist_blacklist();
    }
}

void AuthManager::register_successful_login(const std::string& username, const std::string& ip) {
    string key = make_key(username, ip);
    lock_guard<mutex> g(attempts_mtx_);
    attempts_.erase(key);
}

bool AuthManager::ldap_check_credentials(const std::string& username,
                                         const std::string& password,
                                         std::string& err)
{
    // ----------------------------
    // LDAP Configuration
    // ----------------------------
    const std::string ldapUri = "ldap://ldap.technikum-wien.at:389";
    const int ldapVersion = LDAP_VERSION3;

    std::string bindDn = "uid=" + username + ",ou=people,dc=technikum-wien,dc=at";

    // ----------------------------
    // Initialize LDAP
    // ----------------------------
    LDAP* ld = nullptr;
    int rc = ldap_initialize(&ld, ldapUri.c_str());
    if (rc != LDAP_SUCCESS) {
        std::cerr << "ldap_initialize failed: " << ldap_err2string(rc) << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "connected to LDAP server " << ldapUri << std::endl;

    rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &ldapVersion);
    if (rc != LDAP_OPT_SUCCESS) {
        std::cerr << "ldap_set_option(PROTOCOL_VERSION): " << ldap_err2string(rc) << std::endl;
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        return EXIT_FAILURE;
    }

    // ----------------------------
    // Start TLS
    // ----------------------------
    rc = ldap_start_tls_s(ld, nullptr, nullptr);
    if (rc != LDAP_SUCCESS) {
        std::cerr << "ldap_start_tls_s(): " << ldap_err2string(rc) << std::endl;
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        return EXIT_FAILURE;
    }

    // ----------------------------
    // Bind credentials
    // ----------------------------
    BerValue cred;
    cred.bv_val = const_cast<char*>(password.c_str());
    cred.bv_len = password.size();
    BerValue* servercredp = nullptr;

    rc = ldap_sasl_bind_s(ld, bindDn.c_str(), LDAP_SASL_SIMPLE, &cred, nullptr, nullptr, &servercredp);
    if (rc != LDAP_SUCCESS) {
        std::cerr << "LDAP bind error: " << ldap_err2string(rc) << std::endl;
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        return EXIT_FAILURE;
    }

    return true;
}

//
//  AUTHENTICATE WRAPPER
//
bool AuthManager::authenticate(const std::string& username,
                               const std::string& password,
                               const std::string& client_ip,
                               std::string& err)
{
    blacklist_->garbage_collect();

    if (blacklist_->is_blocked(client_ip)) {
        err = "IP blocked";
        return false;
    }

    string key = make_key(username, client_ip);

    {
        lock_guard<mutex> g(attempts_mtx_);
        auto it = attempts_.find(key);
        if (it != attempts_.end()) {
            if (it->second.second > clk::now()) {
                err = "blocked due to repeated failed attempts";
                blacklist_->add(client_ip, it->second.second);
                persist_blacklist();
                return false;
            }
        }
    }

    // Try LDAP bind
    if (ldap_check_credentials(username, password, err)) {
        register_successful_login(username, client_ip);
        return true;
    }

    // On failure
    register_failed_attempt(username, client_ip);
    return false;
}

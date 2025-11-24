#include "AuthManager.hpp"
#include "Blacklist.hpp"

// Creates a unique key from username and IP for tracking attempts
static string make_key(const string& u, const string& ip) {
    return u + "|" + ip;
}

// Constructor: initializes LDAP settings and loads the blacklist
AuthManager::AuthManager(const string& ldap_uri,
                         const string& search_base,
                         const string& blacklist_file)
    : ldap_uri_(ldap_uri),
      search_base_(search_base),
      blacklist_file_(blacklist_file)
{
    blacklist_ = new Blacklist(blacklist_file_);
    blacklist_->load();  // Load persisted blacklist entries
}

// Destructor: persists blacklist and releases memory
AuthManager::~AuthManager() {
    persist_blacklist();
    delete blacklist_;
}

// Checks whether the given IP is currently blacklisted
bool AuthManager::is_ip_blacklisted(const string& ip) {
    blacklist_->garbage_collect();  // Remove expired blocks
    return blacklist_->is_blocked(ip);
}

// Saves blacklist to disk after cleaning old entries
void AuthManager::persist_blacklist() {
    blacklist_->garbage_collect();
    blacklist_->save();
}

// Records a failed login attempt and blocks IP if threshold is exceeded
void AuthManager::register_failed_attempt(const string& username, const string& ip) {
    string key = make_key(username, ip);
    lock_guard<mutex> g(attempts_mtx_);

    auto& pair = attempts_[key];     // pair.first = fail count, pair.second = block-until time
    pair.first += 1;                 // Increment fail counter

    // If failures exceed threshold, block for 1 minute
    if (pair.first >= 3) {
        auto until = clk::now() + minutes(1);
        pair.second = until;
        blacklist_->add(ip, until);  // Add IP to blacklist
        pair.first = 0;              // Reset counter after blocking
        persist_blacklist();
    }
}

// Clears login attempt counter for the user/IP pair after success
void AuthManager::register_successful_login(const string& username, const string& ip) {
    string key = make_key(username, ip);
    lock_guard<mutex> g(attempts_mtx_);
    attempts_.erase(key);  // Remove attempt tracking entry
}

// Performs LDAP authentication using username/password
bool AuthManager::ldap_check_credentials(const string& username,
                                         const string& password,
                                         string& err)
{
    // LDAP Configuration (hardcoded URI + settings)
    const string ldapUri = "ldap://ldap.technikum-wien.at:389";
    const int ldapVersion = LDAP_VERSION3;

    // Construct bind DN for user
    string bindDn = "uid=" + username + ",ou=people,dc=technikum-wien,dc=at";

    // Initialize LDAP connection
    LDAP* ld = nullptr;
    int rc = ldap_initialize(&ld, ldapUri.c_str());
    if (rc != LDAP_SUCCESS) {
        cerr << "ldap_initialize failed: " << ldap_err2string(rc) << endl;
        return false;
    }
    cout << "connected to LDAP server " << ldapUri << endl;

    // Set LDAP protocol version
    rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &ldapVersion);
    if (rc != LDAP_OPT_SUCCESS) {
        cerr << "ldap_set_option(PROTOCOL_VERSION): "
             << ldap_err2string(rc) << endl;
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        return false;
    }

    // Upgrade connection to TLS
    rc = ldap_start_tls_s(ld, nullptr, nullptr);
    if (rc != LDAP_SUCCESS) {
        cerr << "ldap_start_tls_s(): " << ldap_err2string(rc) << endl;
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        return false;
    }

    // Prepare password for simple bind
    BerValue cred;
    cred.bv_val = const_cast<char*>(password.c_str());
    cred.bv_len = password.size();
    BerValue* servercredp = nullptr;

    // Attempt LDAP bind
    rc = ldap_sasl_bind_s(ld, bindDn.c_str(), LDAP_SASL_SIMPLE,
                          &cred, nullptr, nullptr, &servercredp);

    if (rc != LDAP_SUCCESS) {
        cerr << "LDAP bind error: " << ldap_err2string(rc) << endl;
        ldap_unbind_ext_s(ld, nullptr, nullptr);
        return false;
    }

    return true;  // Credentials valid
}

// Main authentication method combining blacklist checks + LDAP login
bool AuthManager::authenticate(const string& username,
                               const string& password,
                               const string& client_ip,
                               string& err)
{
    blacklist_->garbage_collect();

    // Check IP-level blacklist
    if (blacklist_->is_blocked(client_ip)) {
        err = "IP blocked";
        return false;
    }

    string key = make_key(username, client_ip);

    // Check if user/IP pair is still under a temporary block
    {
        lock_guard<mutex> g(attempts_mtx_);
        auto it = attempts_.find(key);
        if (it != attempts_.end()) {

            // it->second.second = unblock time
            if (it->second.second > clk::now()) {
                err = "blocked due to repeated failed attempts";

                // Ensure IP is also listed in blacklist
                blacklist_->add(client_ip, it->second.second);
                persist_blacklist();
                return false;
            }
        }
    }

    // Attempt LDAP authentication
    if (ldap_check_credentials(username, password, err)) {
        register_successful_login(username, client_ip);
        return true;
    }

    // Record failed attempt on bad password
    register_failed_attempt(username, client_ip);
    return false;
}

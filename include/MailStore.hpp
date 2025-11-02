#ifndef MAILSTORE_HPP
#define MAILSTORE_HPP

#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>

using std::string;
using std::vector;
using std::optional;
using std::nullopt;
using std::ostringstream;
using std::setw;
using std::setfill;
using std::ios;
using std::ofstream;
using std::ifstream;
using std::exception;

struct Message {
    string filename; // internal filename
    string sender;
    string receiver;
    string subject;
    string body; // full body (can include newlines)
};

class MailStore {
public:
    MailStore(const string &spool_dir);
    bool ensure_spool_ok(string &err);
    // Store a message for receiver; returns true on success
    bool store_message(const Message &msg, string &err);
    // List subjects for a user (ordered by filename ascending)
    bool list_subjects(const string &user, vector<string> &subjects);
    // Read the Nth message (1-based). Returns nullopt on error.
    optional<Message> read_message(const string &user, size_t index);
    // Delete the Nth message (1-based). Returns true on success.
    bool delete_message(const string &user, size_t index, string &err);

private:
    string spool;
    string user_inbox_path(const string &user) const;
    bool ensure_user_inbox(const string &user, string &err);
};

#endif // MAILSTORE_HPP

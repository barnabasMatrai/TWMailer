#ifndef MAILSTORE_HPP
#define MAILSTORE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <optional>

using std::endl;
using std::string;
using std::vector;
using std::optional;
using std::nullopt;
using std::ostringstream;
using std::ofstream;
using std::ifstream;
using std::ios;
using std::exception;

struct Message {
    string filename;
    string sender;
    string receiver;
    string subject;
    string body;
};

class MailStore {
public:
    MailStore(const string& spoolDir);
    ~MailStore();

    bool ensure_spool_ok(string& err);

    // store message; returns true on success
    bool store_message(const Message& msg, string& err);

    // list subjects for user (in chronological order)
    bool list_subjects(const string& user, vector<string>& subjects);

    // read nth message (1-based) for user
    optional<Message> read_message(const string& user, size_t id);

    // delete nth message (1-based) for user
    bool delete_message(const string& user, size_t id, string& err);

private:
    string spoolDir;
    string user_dir(const string& user) const;
    vector<string> list_user_files(const string& user) const;
};

#endif // MAILSTORE_HPP

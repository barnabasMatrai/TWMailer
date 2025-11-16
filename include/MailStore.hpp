#ifndef MAILSTORE_HPP
#define MAILSTORE_HPP

#include <string>
#include <vector>
#include <optional>

struct Message {
    std::string filename; // internal filename
    std::string sender;
    std::string receiver;
    std::string subject;
    std::string body;
};

class MailStore {
public:
    MailStore(const std::string& spoolDir);
    ~MailStore();

    bool ensure_spool_ok(std::string& err);

    // store message; returns true on success
    bool store_message(const Message& msg, std::string& err);

    // list subjects for user (in chronological order)
    bool list_subjects(const std::string& user, std::vector<std::string>& subjects);

    // read nth message (1-based) for user
    std::optional<Message> read_message(const std::string& user, size_t idx);

    // delete nth message (1-based) for user
    bool delete_message(const std::string& user, size_t idx, std::string& err);

private:
    std::string spoolDir;
    std::string user_dir(const std::string& user) const;
    std::vector<std::string> list_user_files(const std::string& user) const;
};

#endif // MAILSTORE_HPP

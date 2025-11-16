#include "MailStore.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

namespace fs = std::filesystem;

MailStore::MailStore(const std::string& spoolDir) : spoolDir(spoolDir) {}

MailStore::~MailStore() {}

bool MailStore::ensure_spool_ok(std::string& err) {
    try {
        if (!fs::exists(spoolDir)) {
            fs::create_directories(spoolDir);
        } else if (!fs::is_directory(spoolDir)) {
            err = "spool path exists but is not a directory";
            return false;
        }
    } catch (const fs::filesystem_error& e) {
        err = e.what();
        return false;
    }
    return true;
}

std::string MailStore::user_dir(const std::string& user) const {
    return spoolDir + "/" + user;
}

static std::string make_timestamp_filename() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
    std::ostringstream ss;
    ss << ms;
    return ss.str();
}

bool MailStore::store_message(const Message& msg, std::string& err) {
    try {
        std::string udir = user_dir(msg.receiver);
        if (!fs::exists(udir)) fs::create_directories(udir);
        std::string fname = make_timestamp_filename() + ".msg";
        std::string full = udir + "/" + fname;
        std::ofstream ofs(full, std::ofstream::trunc | std::ofstream::binary);
        if (!ofs.is_open()) {
            err = "failed to open message file for writing";
            return false;
        }
        // simple format:
        // Sender: <sender>\n
        // Receiver: <receiver>\n
        // Subject: <subject>\n
        // Body:\n
        // <body>
        ofs << "Sender: " << msg.sender << "\n";
        ofs << "Receiver: " << msg.receiver << "\n";
        ofs << "Subject: " << msg.subject << "\n";
        ofs << "Body:\n";
        ofs << msg.body;
        ofs.close();
    } catch (const std::exception& e) {
        err = e.what();
        return false;
    }
    return true;
}

std::vector<std::string> MailStore::list_user_files(const std::string& user) const {
    std::vector<std::string> files;
    std::string udir = user_dir(user);
    if (!fs::exists(udir) || !fs::is_directory(udir)) return files;
    for (auto& de : fs::directory_iterator(udir)) {
        if (!de.is_regular_file()) continue;
        files.push_back(de.path().filename().string());
    }
    // sort by filename (timestamps) ascending
    std::sort(files.begin(), files.end());
    return files;
}

bool MailStore::list_subjects(const std::string& user, std::vector<std::string>& subjects) {
    subjects.clear();
    try {
        auto files = list_user_files(user);
        for (auto& f : files) {
            std::string path = user_dir(user) + "/" + f;
            std::ifstream ifs(path, std::ios::binary);
            if (!ifs.is_open()) continue;
            std::string line;
            // read until Subject: <subject>\n
            std::string subject;
            while (std::getline(ifs, line)) {
                if (line.rfind("Subject: ", 0) == 0) {
                    subject = line.substr(9);
                    break;
                }
            }
            if (subject.empty()) subject = "(no subject)";
            subjects.push_back(subject);
        }
    } catch (...) {
        return false;
    }
    return true;
}

std::optional<Message> MailStore::read_message(const std::string& user, size_t idx) {
    auto files = list_user_files(user);
    if (idx == 0 || idx > files.size()) return std::nullopt;
    std::string path = user_dir(user) + "/" + files[idx - 1];
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) return std::nullopt;
    Message m;
    m.filename = files[idx - 1];
    std::string line;
    // read header lines
    while (std::getline(ifs, line)) {
        if (line.rfind("Sender: ", 0) == 0) {
            m.sender = line.substr(8);
        } else if (line.rfind("Receiver: ", 0) == 0) {
            m.receiver = line.substr(10);
        } else if (line.rfind("Subject: ", 0) == 0) {
            m.subject = line.substr(9);
        } else if (line == "Body:") {
            // read rest as body
            std::ostringstream body;
            std::string l;
            while (std::getline(ifs, l)) {
                body << l;
                if (!ifs.eof()) body << "\n";
            }
            m.body = body.str();
            break;
        }
    }
    return m;
}

bool MailStore::delete_message(const std::string& user, size_t idx, std::string& err) {
    auto files = list_user_files(user);
    if (idx == 0 || idx > files.size()) {
        err = "invalid message number";
        return false;
    }
    std::string path = user_dir(user) + "/" + files[idx - 1];
    try {
        fs::remove(path);
    } catch (const std::exception& e) {
        err = e.what();
        return false;
    }
    return true;
}

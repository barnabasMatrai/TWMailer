#include "MailStore.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

namespace fs = std::filesystem;

MailStore::MailStore(const string& spoolDir) : spoolDir(spoolDir) {}

MailStore::~MailStore() {}

bool MailStore::ensure_spool_ok(string& err) {
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

string MailStore::user_dir(const string& user) const {
    return spoolDir + "/" + user;
}

static string make_timestamp_filename() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
    ostringstream ss;
    ss << ms;
    return ss.str();
}

bool MailStore::store_message(const Message& msg, string& err) {
    try {
        string udir = user_dir(msg.receiver);
        if (!fs::exists(udir)) fs::create_directories(udir);
        string fname = make_timestamp_filename() + ".msg";
        string full = udir + "/" + fname;
        ofstream ofs(full, ofstream::trunc | ofstream::binary);
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
        ofs << "Sender: " << msg.sender << endl;
        ofs << "Receiver: " << msg.receiver << endl;
        ofs << "Subject: " << msg.subject << endl;
        ofs << "Body:\n";
        ofs << msg.body;
        ofs.close();
    } catch (const exception& e) {
        err = e.what();
        return false;
    }
    return true;
}

vector<string> MailStore::list_user_files(const string& user) const {
    vector<string> files;
    string udir = user_dir(user);
    if (!fs::exists(udir) || !fs::is_directory(udir)) return files;
    for (auto& de : fs::directory_iterator(udir)) {
        if (!de.is_regular_file()) continue;
        files.push_back(de.path().filename().string());
    }
    // sort by filename (timestamps) ascending
    sort(files.begin(), files.end());
    return files;
}

bool MailStore::list_subjects(const string& user, vector<string>& subjects) {
    subjects.clear();
    try {
        auto files = list_user_files(user);
        for (auto& f : files) {
            string path = user_dir(user) + "/" + f;
            ifstream ifs(path, ios::binary);
            if (!ifs.is_open()) continue;
            string line;
            // read until Subject: <subject>\n
            string subject;
            while (getline(ifs, line)) {
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

optional<Message> MailStore::read_message(const string& user, size_t id) {
    auto files = list_user_files(user);
    if (id == 0 || id > files.size()) return nullopt;
    string path = user_dir(user) + "/" + files[id - 1];
    ifstream ifs(path, ios::binary);
    if (!ifs.is_open()) return nullopt;
    Message m;
    m.filename = files[id - 1];
    string line;
    // read header lines
    while (getline(ifs, line)) {
        if (line.rfind("Sender: ", 0) == 0) {
            m.sender = line.substr(8);
        } else if (line.rfind("Receiver: ", 0) == 0) {
            m.receiver = line.substr(10);
        } else if (line.rfind("Subject: ", 0) == 0) {
            m.subject = line.substr(9);
        } else if (line == "Body:") {
            // read rest as body
            ostringstream body;
            string l;
            while (getline(ifs, l)) {
                body << l;
                if (!ifs.eof()) body << endl;
            }
            m.body = body.str();
            break;
        }
    }
    return m;
}

bool MailStore::delete_message(const string& user, size_t id, string& err) {
    auto files = list_user_files(user);
    if (id == 0 || id > files.size()) {
        err = "invalid message number";
        return false;
    }
    string path = user_dir(user) + "/" + files[id - 1];
    try {
        fs::remove(path);
    } catch (const exception& e) {
        err = e.what();
        return false;
    }
    return true;
}

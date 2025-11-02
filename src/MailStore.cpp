#include "MailStore.hpp"

namespace fs = std::filesystem;

MailStore::MailStore(const string &spool_dir) : spool(spool_dir) {}

bool MailStore::ensure_spool_ok(string &err) {
    try {
        if (!fs::exists(spool)) {
            fs::create_directories(spool);
        }
        if (!fs::is_directory(spool)) {
            err = "Spool path exists and is not a directory";
            return false;
        }
    } catch (const fs::filesystem_error &e) {
        err = e.what();
        return false;
    }
    return true;
}

string MailStore::user_inbox_path(const string &user) const {
    return spool + "/" + user + "/inbox";
}

bool MailStore::ensure_user_inbox(const string &user, string &err) {
    try {
        fs::path p(user_inbox_path(user));
        if (!fs::exists(p)) {
            fs::create_directories(p);
        }
        if (!fs::is_directory(p)) {
            err = "User inbox path exists and is not a directory";
            return false;
        }
    } catch (const fs::filesystem_error &e) {
        err = e.what();
        return false;
    }
    return true;
}

static string make_unique_filename() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
    ostringstream ss;
    ss << t << "_" << setw(3) << setfill('0') << ms;
    return ss.str();
}

bool MailStore::store_message(const Message &msg, string &err) {
    if (!ensure_user_inbox(msg.receiver, err)) {
        return false;
    }
    fs::path inbox(user_inbox_path(msg.receiver));
    string fname = make_unique_filename();
    fs::path file = inbox / (fname + ".msg");
    try {
        ofstream ofs(file, ios::out | ios::binary);
        if (!ofs) {
            err = "Failed to open file for writing";
            return false;
        }
        // simple storage format: lines with headers then body and final newline
        ofs << "Sender: " << msg.sender << "\n";
        ofs << "Receiver: " << msg.receiver << "\n";
        ofs << "Subject: " << msg.subject << "\n";
        ofs << "\n";
        ofs << msg.body;
        if (!msg.body.empty() && msg.body.back() != '\n') {
            ofs << "\n";
        }
        ofs.close();
    } catch (const exception &e) {
        err = e.what();
        return false;
    }
    return true;
}

bool MailStore::list_subjects(const string &user, vector<string> &subjects) {
    subjects.clear();
    fs::path inbox(user_inbox_path(user));
    if (!fs::exists(inbox) || !fs::is_directory(inbox)) {
        return false;
    }
    try {
        // gather filenames and sort
        vector<fs::directory_entry> entries;
        for (auto &ent : fs::directory_iterator(inbox)) {
            if (ent.is_regular_file()) {
                entries.push_back(ent);
            }
        }
        sort(entries.begin(), entries.end(), [](const fs::directory_entry &a, const fs::directory_entry &b){
            return a.path().filename().string() < b.path().filename().string();
        });
        for (auto &ent : entries) {
            ifstream ifs(ent.path());
            if (!ifs) {
                continue;
            }
            string line;
            string subject = "";
            while (getline(ifs, line)) {
                if (line.rfind("Subject:", 0) == 0) {
                    subject = line.substr(string("Subject:").size());
                    // trim leading spaces
                    if (!subject.empty() && subject[0] == ' ') {
                        subject.erase(0,1);
                    }
                    break;
                }
            }
            subjects.push_back(subject);
        }
    } catch (...) {
        return false;
    }
    return true;
}

optional<Message> MailStore::read_message(const string &user, size_t index) {
    fs::path inbox(user_inbox_path(user));
    if (!fs::exists(inbox) || !fs::is_directory(inbox)) {
        return nullopt;
    }
    try {
        vector<fs::directory_entry> entries;
        for (auto &ent : fs::directory_iterator(inbox)) {
            if (ent.is_regular_file()) entries.push_back(ent);
        }
        sort(entries.begin(), entries.end(), [](const fs::directory_entry &a, const fs::directory_entry &b){
            return a.path().filename().string() < b.path().filename().string();
        });
        if (index == 0 || index > entries.size()) {
            return nullopt;
        }
        fs::path file = entries[index-1].path();
        ifstream ifs(file);
        if (!ifs) {
            return nullopt;
        }
        Message msg;
        msg.filename = file.filename().string();
        string line;
        // read headers
        while (getline(ifs, line)) {
            if (line.empty()) break;
            if (line.rfind("Sender:", 0) == 0) {
                msg.sender = line.substr(string("Sender:").size());
            }
            if (line.rfind("Sender:", 0) == 0 && !msg.sender.empty() && msg.sender[0]==' ') {
                msg.sender.erase(0,1);
            }
            if (line.rfind("Receiver:", 0) == 0) {
                msg.receiver = line.substr(string("Receiver:").size());
            }
            if (line.rfind("Receiver:", 0) == 0 && !msg.receiver.empty() && msg.receiver[0]==' ') {
                msg.receiver.erase(0,1);
            }
            if (line.rfind("Subject:", 0) == 0) {
                msg.subject = line.substr(string("Subject:").size());
            }
            if (line.rfind("Subject:", 0) == 0 && !msg.subject.empty() && msg.subject[0]==' ') {
                msg.subject.erase(0,1);
            }
        }
        // rest is body
        ostringstream body;
        bool first = true;
        while (getline(ifs, line)) {
            if (!first) {
                body << "\n";
            }
            body << line;
            first = false;
        }
        msg.body = body.str();
        return msg;
    } catch (...) {
        return nullopt;
    }
}

bool MailStore::delete_message(const string &user, size_t index, string &err) {
    fs::path inbox(user_inbox_path(user));
    if (!fs::exists(inbox) || !fs::is_directory(inbox)) {
        err = "User not found";
        return false;
    }
    try {
        vector<fs::directory_entry> entries;
        for (auto &ent : fs::directory_iterator(inbox)) {
            if (ent.is_regular_file()) {
                entries.push_back(ent);
            }
        }
        sort(entries.begin(), entries.end(), [](const fs::directory_entry &a, const fs::directory_entry &b){
            return a.path().filename().string() < b.path().filename().string();
        });
        if (index == 0 || index > entries.size()) {
            err = "Message number out of range";
            return false;
        }
        fs::remove(entries[index-1].path());
        return true;
    } catch (const fs::filesystem_error &e) {
        err = e.what();
        return false;
    }
}

#include "Blacklist.hpp"

// Convert time_point → milliseconds since epoch (for file storage)
static long long tp_to_ms(const clk::time_point& tp) {
    return duration_cast<milliseconds>(tp.time_since_epoch()).count();
}

// Convert milliseconds since epoch → time_point (for loading)
static clk::time_point ms_to_tp(long long ms) {
    return clk::time_point(milliseconds(ms));
}

// Constructor: stores the path to the blacklist file
Blacklist::Blacklist(const string& file_path) : file_path_(file_path) {}

// Destructor: persist blacklist on destruction
Blacklist::~Blacklist() { save(); }

// Load blacklist file into memory
bool Blacklist::load() {
    lock_guard<mutex> guard(mtx_);

    ifstream ifs(file_path_);
    if (!ifs.is_open()) {
        return false;  // No file or cannot open
    }

    map_.clear();
    string line;

    // File format: "<ip> <expiry_ms>"
    while (getline(ifs, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string ip;
        long long ms;

        if (!(iss >> ip >> ms)) {
            continue;  // Skip malformed lines
        }

        map_[ip] = ms_to_tp(ms);
    }

    return true;
}

// Save current blacklist to disk (overwrites file)
bool Blacklist::save() {
    lock_guard<mutex> guard(mtx_);

    ofstream ofs(file_path_, ofstream::trunc);
    if (!ofs.is_open()) {
        return false;
    }

    // Write each IP + expiry time in ms
    for (auto& p : map_) {
        ofs << p.first << " " << tp_to_ms(p.second) << endl;
    }

    return true;
}

// Add or update an IP block with an expiration timestamp
void Blacklist::add(const string& ip, const clk::time_point& until) {
    lock_guard<mutex> guard(mtx_);
    map_[ip] = until;
}

// Remove expired entries from the blacklist
void Blacklist::garbage_collect() {
    lock_guard<mutex> guard(mtx_);
    auto now = clk::now();

    // Erase entries whose timestamp is in the past
    for (auto iterator = map_.begin(); iterator != map_.end(); ) {
        if (iterator->second <= now) {
            iterator = map_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

// Check whether an IP is currently blocked
bool Blacklist::is_blocked(const string& ip) {
    lock_guard<mutex> guard(mtx_);

    auto iterator = map_.find(ip);
    if (iterator == map_.end()) {
        return false;  // IP not in blacklist
    }

    // If expired, remove and treat as not blocked
    if (iterator->second <= clk::now()) {
        map_.erase(iterator);
        return false;
    }

    return true;  // IP is still blocked
}

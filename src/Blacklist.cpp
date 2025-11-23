#include "Blacklist.hpp"

static long long tp_to_ms(const clk::time_point& tp) {
    return duration_cast<milliseconds>(tp.time_since_epoch()).count();
}
static clk::time_point ms_to_tp(long long ms) {
    return clk::time_point(milliseconds(ms));
}

Blacklist::Blacklist(const string& file_path) : file_path_(file_path) {}

Blacklist::~Blacklist() { save(); }

bool Blacklist::load() {
    lock_guard<mutex> guard(mtx_);
    ifstream ifs(file_path_);
    if (!ifs.is_open()) {
        return false;
    }
    map_.clear();
    string line;
    while (getline(ifs, line)) {
        if (line.empty()) {
            continue;
        }
        istringstream iss(line);
        string ip;
        long long ms;
        if (!(iss >> ip >> ms)) {
            continue;
        }
        map_[ip] = ms_to_tp(ms);
    }
    return true;
}

bool Blacklist::save() {
    lock_guard<mutex> guard(mtx_);
    ofstream ofs(file_path_, ofstream::trunc);
    if (!ofs.is_open()) {
        return false;
    }
    for (auto& p : map_) {
        ofs << p.first << " " << tp_to_ms(p.second) << endl;
    }
    return true;
}

void Blacklist::add(const string& ip, const clk::time_point& until) {
    lock_guard<mutex> guard(mtx_);
    map_[ip] = until;
}

void Blacklist::garbage_collect() {
    lock_guard<mutex> guard(mtx_);
    auto now = clk::now();
    for (auto iterator = map_.begin(); iterator != map_.end(); ) {
        if (iterator -> second <= now) {
            iterator = map_.erase(iterator);
        }
        else {
            ++iterator;
        }
    }
}

bool Blacklist::is_blocked(const string& ip) {
    lock_guard<mutex> guard(mtx_);
    auto iterator = map_.find(ip);
    if (iterator == map_.end()) {
        return false;
    }
    if (iterator -> second <= clk::now()) {
        map_.erase(iterator);
        return false;
    }
    return true;
}

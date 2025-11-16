#include "Blacklist.hpp"
#include <fstream>
#include <sstream>
#include <chrono>

using clk = std::chrono::system_clock;

static long long tp_to_ms(const clk::time_point& tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
}
static clk::time_point ms_to_tp(long long ms) {
    return clk::time_point(std::chrono::milliseconds(ms));
}

Blacklist::Blacklist(const std::string& file_path) : file_path_(file_path) {}

Blacklist::~Blacklist() { save(); }

bool Blacklist::load() {
    std::lock_guard<std::mutex> g(mtx_);
    std::ifstream ifs(file_path_);
    if (!ifs.is_open()) return false;
    map_.clear();
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string ip;
        long long ms;
        if (!(iss >> ip >> ms)) continue;
        map_[ip] = ms_to_tp(ms);
    }
    return true;
}

bool Blacklist::save() {
    std::lock_guard<std::mutex> g(mtx_);
    std::ofstream ofs(file_path_, std::ofstream::trunc);
    if (!ofs.is_open()) return false;
    for (auto& p : map_) {
        ofs << p.first << " " << tp_to_ms(p.second) << "\n";
    }
    return true;
}

void Blacklist::add(const std::string& ip, const clk::time_point& until) {
    std::lock_guard<std::mutex> g(mtx_);
    map_[ip] = until;
}

void Blacklist::garbage_collect() {
    std::lock_guard<std::mutex> g(mtx_);
    auto now = clk::now();
    for (auto it = map_.begin(); it != map_.end(); ) {
        if (it->second <= now) it = map_.erase(it);
        else ++it;
    }
}

bool Blacklist::is_blocked(const std::string& ip) {
    std::lock_guard<std::mutex> g(mtx_);
    auto it = map_.find(ip);
    if (it == map_.end()) return false;
    if (it->second <= clk::now()) {
        map_.erase(it);
        return false;
    }
    return true;
}

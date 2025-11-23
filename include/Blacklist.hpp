#ifndef BLACKLIST_HPP
#define BLACKLIST_HPP

#include <string>
#include <chrono>
#include <map>
#include <mutex>
#include <iostream>
#include <fstream>
#include <sstream>

using std::endl;
using std::string;
using std::map;
using std::mutex;
using std::lock_guard;
using std::ifstream;
using std::ofstream;
using std::istringstream;
using clk = std::chrono::system_clock;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

class Blacklist {
public:
    Blacklist(const string& file_path);
    ~Blacklist();

    bool load(); // load from file
    bool save(); // save to file

    void add(const string& ip, const system_clock::time_point& until);
    bool is_blocked(const string& ip);
    void garbage_collect();

private:
    map<string, system_clock::time_point> map_;
    mutex mtx_;
    string file_path_;
};

#endif // BLACKLIST_HPP

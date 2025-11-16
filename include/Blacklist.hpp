#ifndef BLACKLIST_HPP
#define BLACKLIST_HPP

#include <string>
#include <chrono>
#include <map>
#include <mutex>

class Blacklist {
public:
    Blacklist(const std::string& file_path);
    ~Blacklist();

    bool load(); // load from file
    bool save(); // save to file

    void add(const std::string& ip, const std::chrono::system_clock::time_point& until);
    bool is_blocked(const std::string& ip);
    void garbage_collect();

private:
    std::map<std::string, std::chrono::system_clock::time_point> map_;
    std::mutex mtx_;
    std::string file_path_;
};

#endif // BLACKLIST_HPP

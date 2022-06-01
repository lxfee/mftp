#include "utils.h"
#include "logger.hpp"
#include <sstream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

int getrandom(int l, int r) {
    static std::random_device dev;
    static std::mt19937 rng(dev());
    std::uniform_int_distribution<int> dist(l, r); // distribution in range [1, 6]
    return dist(rng);
}

std::string getcurrenttime() {
    // 基于当前系统的当前日期/时间
    time_t now = time(0);    
    tm *ltm = localtime(&now);
    std::string res;
    std::stringstream timestr;

    // 输出 tm 结构的各个组成部分
    timestr << 1900 + ltm->tm_year << "Y ";
    timestr << 1 + ltm->tm_mon << "M ";
    timestr <<  ltm->tm_mday << "D ";
    timestr << ltm->tm_hour << ":";
    timestr << ltm->tm_min << ":";
    timestr << ltm->tm_sec << "\n";
    getline(timestr, res);
    logger(res, "TIME");
    return res;
}
    

bool direxists(std::string path) {
    using namespace fs;
    return exists(path) && is_directory(path);
}

bool fileexists(std::string path) {
    using namespace fs;
    return exists(path) && is_regular_file(path);
}

bool pathexists(std::string path) {
    using namespace fs;
    return exists(path);
}

uintmax_t computefilesize(fs::path path) {
    using namespace fs;
    if(exists(path) && is_regular_file(path)) {
        auto err = std::error_code{};
        auto filesize = file_size(path, err);
        if (filesize != static_cast<uintmax_t>(-1)) return filesize;
    }
    return static_cast<uintmax_t>(-1);
}

std::string walkpath(std::string base, std::string path) {
    if(path.empty()) return base;
    if(!base.empty() && base.back() == '/') base.pop_back();
    
    std::vector<std::string> stpath;
    std::reverse(path.begin(), path.end());

    while(!path.empty()) {
        while(!path.empty() && path.back() == '/') path.pop_back();
        std::string obj;
        while(!path.empty() && path.back() != '/') {
            obj.push_back(path.back());
            path.pop_back();
        }
        if(obj.empty()) continue;
        if(obj == "..") {
            if(!stpath.empty()) stpath.pop_back();
        } else if(obj != ".") {
            stpath.push_back(obj);
        }
    }

    std::string tpath;
    for(int i = 0; i < stpath.size(); i++) {
        tpath.push_back('/');
        tpath.append(stpath[i]);            
    }
    return base + tpath;
}

bool getlist(std::string path, std::ostream &list) {
    std::vector<std::pair<int, fs::path>> paths;

    if(direxists(path)) {
        for (const auto& entry : fs::directory_iterator(path)) {
            auto epath = entry.path();
            if (fs::is_directory(entry.status())) {
                paths.push_back(std::make_pair(0, epath));
            }
            else if (fs::is_regular_file(entry.status())) {
                paths.push_back(std::make_pair(1, epath));
            } else {
                paths.push_back(std::make_pair(2, epath));
            }
        }
    } else if(fs::is_regular_file(path)) {
        paths.push_back(std::make_pair(1, fs::path(path)));
    } else {
        return false;
    }

    std::sort(paths.begin(), paths.end());
    int wide = 0;
    for(int i = 0; i < paths.size(); i++) {
        wide = std::max(wide, (int)paths[i].second.filename().string().size());
    }
    wide = std::max(wide, 4);
    wide += 2;
    list << "[type]  ";
    list << std::setw(20) << "size";
    list << std::setw(wide) << "name";
    list << std::endl;
    for(const auto& ppath : paths) {
        switch(ppath.first) {
            case 0:
                list << "[D]     ";
                list << std::setw(20) << " ";
                list << std::setw(wide) << ppath.second.filename().string();
                list << std::endl;
                break;
            case 1:
                list << "[F]     ";
                list << std::setw(20) << computefilesize(ppath.second);
                list << std::setw(wide) << ppath.second.filename().string();
                list << std::endl;
                break;
            case 2:
            default:
                list << "[?]     ";
                list << std::setw(20) << " ";
                list << std::setw(wide) << ppath.second.filename().string();
                list << std::endl;
                break;
        }
    }
    return true;
}


bool parseIp(Ipaddr& addr, std::string ip) {
    int p = -1;
    for(int i = 0; i < ip.size(); i++) {
        if(ip[i] == ':') {
            p = i;
            break;
        }
    }
    if(p == -1) return false;
    std::string ipaddr = ip.substr(0, p);
    std::string port = ip.substr(p + 1);
    addr.port = atoi(port.c_str());
    addr.setaddr(ipaddr);
    return true;
}


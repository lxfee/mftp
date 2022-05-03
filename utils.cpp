// need C++ 17
#include "utils.h"
#include "logger.hpp"
#include <sstream>
#include <filesystem>

std::string getcurrenttime() {
    // 基于当前系统的当前日期/时间
    time_t now = time(0);    
    tm *ltm = localtime(&now);
    std::string res;
    std::stringstream timestr;

    // 输出 tm 结构的各个组成部分
    timestr << 1900 + ltm->tm_year << "年";
    timestr << 1 + ltm->tm_mon << "月";
    timestr <<  ltm->tm_mday << "日 ";
    timestr << ltm->tm_hour << ":";
    timestr << ltm->tm_min << ":";
    timestr << ltm->tm_sec << "\n";
    getline(timestr, res);
    logger(res, "时间");
    return res;
}
    
namespace fs = std::filesystem;

bool direxists(std::string path) {
    return fs::exists(path) && fs::is_directory(path);
}

uintmax_t ComputeFileSize(const fs::path& pathToCheck) {
    if (fs::exists(pathToCheck) && fs::is_regular_file(pathToCheck)) {
        auto err = std::error_code{};
        auto filesize = fs::file_size(pathToCheck, err);
        if (filesize != static_cast<uintmax_t>(-1)) return filesize;
    }
    return static_cast<uintmax_t>(-1);
}


bool getlist(std::string path, std::ostream &list) {
    if(direxists(path)) {
        for (const auto& entry : fs::directory_iterator(path)) {
            auto filename = entry.path().filename().string();
            if (fs::is_directory(entry.status())) {
                list << "[D] ";
                list << std::setw(16) << " ";
                list << std::setw(20) << filename;
                list << std::endl;
            }
            else if (fs::is_regular_file(entry.status())) {
                list << "[F] ";
                list << std::setw(16) << ComputeFileSize(entry.path());
                list << std::setw(20) << filename;
                list << std::endl;
            } else {
                list << "[?] ";
                list << std::setw(16) << " ";
                list << std::setw(20) << filename;
                list << std::endl;
            }
        }
    } else if(fs::is_regular_file(path)) {
        list << "[F] ";
        list << std::setw(16) << ComputeFileSize(path);
        list << std::setw(20) << fs::path(path).filename().string();
        list << std::endl;
    } else {
        return false;
    }
    return true;
}

// only support ipv4
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
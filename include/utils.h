#pragma once
// need C++ 17
#include <filesystem>
#include <string>
#include <iostream>
#include "socket.h"

namespace fs = std::filesystem;

/****** 获得信息 *******/
std::string getcurrenttime();
bool getlist(std::string path, std::ostream &list);
uintmax_t computefilesize(fs::path path);
std::string walkpath(std::string base, std::string path);


/***** 判断存在 *******/
bool direxists(std::string path);
bool fileexists(std::string path);
bool pathexists(std::string path);


/***** 解析数据 ********/
// only support ipv4
bool parseIp(Ipaddr& addr, std::string ip);


/******* 其他 *********/
int getrandom(int l, int r);
#pragma once
#include <ctime>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "socket.h"

std::string getcurrenttime();

std::string getpath(std::string base, std::string path);

bool getlist(std::string path, std::ostream &list);

bool direxists(std::string path);

// only support ipv4
bool parseIp(Ipaddr& addr, std::string ip);

int getrandom(int l, int r);
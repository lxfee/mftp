#include <ctime>
#include <string>
#include <iostream>
#include "socket.h"

std::string getcurrenttime();

bool getlist(std::string path, std::ostream &list);

bool direxists(std::string path);

// only support ipv4
bool parseIp(Ipaddr& addr, std::string ip);
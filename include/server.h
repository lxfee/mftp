#pragma once
#include "socket.h"
#include "session.h"
#include <map>

#define CMDPORT  114
#define DATAPORT 514
#define cmderror(respond) {scmd.sendmsg(respond); return -1;} 
#define neterror(msg) {std::cerr << msg << std::endl; return -1;}

struct ServerConfig {
public:
    ServerConfig();
    int loadconfig(std::string filename);
    friend class Server; 
private:
    std::string path;
    std::map<std::string, std::string> users;
    bool allowAnonymous;
};

class Server {
public:
    int operator()(const Ipaddr& clientaddr, const Socket& cmdsock);
private:
    int login(Session& scmd);
    static ServerConfig config; 
    std::string user;
    std::string passwd;
};


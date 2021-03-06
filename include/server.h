#pragma once
#include "session.h"
#include <map>

#define CMDPORT  11451

struct ServerConfig {
    ServerConfig();
    bool loadconfig(std::string filename);
    std::string path;
    std::map<std::string, std::string> users;
    bool allowAnonymous;
    Ipaddr addr;
};

class Server {
public:
    void operator()(Session& scmd);
    static ServerConfig config; 
private:
    bool login(Session& scmd);
    void list(Session& scmd);
    void getfile(Session& scmd);
    void putfile(Session& scmd);
    void runcmd(Session& scmd);
    Session buildstream(Session& scmd);
    std::string user;
    std::string passwd;
};


#include "session.h"
#include "server.h"
#include "json.hpp"
#include <fstream>
#include <dirent.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include "logger.hpp"

/***********************server config****************************/

using json = nlohmann::json;

// only support ipv4
static bool parseIp(Ipaddr& addr, std::string ip) {
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

ServerConfig::ServerConfig() {
    std::ifstream fin;
    fin.open("./config.json");
    if(!fin) {
        fin.close();
        std::ofstream fout;
        fout.open("./config.json");
        if(!fout) {
            panic("[Server Config] can't not create config file!");
        }
        json config;
        config["path"] = "./";
        config["users"]["root"] = "123456";
        config["allow anonymous"] = true;
        config["ip"] = "0.0.0.0:" + std::to_string(CMDPORT);
        fout << std::setw(4) << config;
        fout.close();
        if(loadconfig("./config.json") < 0) {
            panic("[Server Config] can't not parse config file!");
        }
    } else {
        if(loadconfig("./config.json") < 0) {
            panic("[Server Config] can't not parse config file!");
        }
    }
}

bool ServerConfig::loadconfig(std::string filename) {
    std::ifstream fin;
    fin.open(filename);
    if(!fin) {
        logger("can't not open config file!", "Server Config");
        return false;
    }
    json info;
    try {
        fin >> info;
    } catch(nlohmann::detail::parse_error e) {
        logger("config parse error!", "Server Config");
        return false;
    }
    std::string loadedpath;
    std::map<std::string, std::string> loadedusers;
    
    if(info.contains("path")) {
        loadedpath = info["path"];
    } else {
        logger("lack path!", "Server Config");
        return false;
    }

    if(info.contains("users")) {
        for(auto& user : info["users"].items()) {
            loadedusers[user.key()] = user.value();
        }
    } else {
        logger("lack users!", "Server Config");
        return false;
    }

    if(info.contains("allow anonymous")) {
        allowAnonymous = info["allow anonymous"];
    } else {
        logger("anonymous?", "Server Config");
        return false;
    }

    if(info.contains("ip")) {
        if(!parseIp(addr, info["ip"])) {
            logger("can not parse ip", "Server Config");
            return false;
        }
    } else {
        logger("where is ip", "Server Config");
        return false;
    }

    path = loadedpath;
    users = loadedusers;
    return true;
}

ServerConfig Server::config; 


/***********************server****************************/

void Server::operator()(Session& scmd) {
    // init
    if(!login(scmd)) return ;

    std::string cmd;
    while(1) {
        scmd.nextline();
        scmd.gettok(cmd);

        if(cmd == "BYE") {
            scmd.sendmsg("BYE");
            break;
        }
        else 
        if(cmd == "LIST") {
            list(scmd);
            scmd.sendmsg("DONE");
        }
        else
        if(cmd == "GET") {

        }
        else
        if(cmd == "PUT") {

        }
        else 
        {
            scmd.sendmsg("ERR: unknown cmd");
        }
    }
}

bool getlistinfo(std::string path, std::string &listinfo) {
    std::string test;
    test += "123\n";
    test += "123\n";
    test += "123\n";
    test += "123\n";
    test += "123\n";
    test += "123\n";
    listinfo = test;
    return true;
}

void Server::list(Session& scmd) {
    std::string path;
    scmd.gettok(path);

    std::string listinfo;
    
    if(getlistinfo(path, listinfo)) {
        scmd.sendmsg("OK");
    } else {
        scmd.sendmsg("ERR: Path not exisit");
        return ;
    }
    std::stringstream ss(listinfo);
    Session datasession = buildstream(scmd);
    if(!datasession.status()) return ;

    scmd.sendmsg("Start send binary stream");
    datasession.sendstream(ss);

}

Session Server::buildstream(Session& scmd) {
    std::string cmd;
    
    scmd.nextline();
    scmd.gettok(cmd);

    if(cmd == "PORT") {
        scmd.gettok(cmd);
        Ipaddr target = scmd.gettargetaddr();
        target.port = atoi(cmd.c_str()); 
        
        Ipaddr local = scmd.getlocaladdr();
        local.port++; // 数据端口 = 命令端口 + 1
        Session session = Session::buildsession(target, local, ACTIVE);
        if(!session.status()) {
            scmd.sendmsg("ERR: can not build data connection");
        }
        return std::move(session);

    } else if(cmd == "PASV") {
        Ipaddr local = scmd.getlocaladdr();
        local.port = 0;
        Session session = Session::buildlocalsession(local, CLOSE);

        if(!session.status()) {
            scmd.sendmsg("ERR: can not build data connection");
            return std::move(session);
        }
        
        session.listen(1);
        scmd.sendmsg("PORT " + std::to_string(session.getlocaladdr().port));

        Session dsession = session.accept(5, ACTIVE);
        if(dsession.status() < 0) {
            scmd.sendmsg("ERR: data connection time out");
        }
        // 建立会话
        return std::move(dsession);

    } else {
        scmd.sendmsg("ERR: do not know which way");
        return Session::nullsession();
    }
}

bool Server::login(Session& scmd) {
    std::string cmd;
    scmd.sendmsg("LOGIN");

    // USER
    scmd.nextline();
    scmd.gettok(cmd);

    if(cmd != "USER") {
        scmd.sendmsg("ERR: request error");
        return false;
    }
    scmd.gettok(cmd);
    if(cmd == "anonymous" && !config.allowAnonymous){
        scmd.sendmsg("ERR: request error");
        return false;
    }
    if(cmd != "anonymous" && !config.users.count(cmd)) {
        scmd.sendmsg("ERR: user not exist!");
        return false;
    }
    user = cmd;
    scmd.sendmsg("OK");
    
    
    // PASSWORD 
    scmd.nextline();
    scmd.gettok(cmd);
    if(cmd != "PASSWORD") {
        scmd.sendmsg("ERR: request error");
        return false;
    }
    scmd.gettok(cmd);
    if(user != "anonymous" && config.users.at(user) != cmd)  {
        scmd.sendmsg("ERR: authentication failed");
        return false;
    }
    passwd = cmd;
    scmd.sendmsg("OK");

    // DONE
    return true;
}


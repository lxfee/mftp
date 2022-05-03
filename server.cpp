#include "session.h"
#include "server.h"
#include "json.hpp"
#include <fstream>
#include <dirent.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include "logger.hpp"
#include <fstream>
#include "utils.h"

/***********************server config****************************/

using json = nlohmann::json;
namespace fs = std::filesystem;

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
    if(!direxists(loadedpath)) {
        logger("can not open dir!", "Server Config");
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
        else if(cmd == "LIST") {
            list(scmd);
        }
        else if(cmd == "GET") {
            getfile(scmd);
        }
        else if(cmd == "PUT") {

        }
        else if(cmd == "TIME") {
            scmd.sendmsg(getcurrenttime());
        } 
        else if(cmd == "HELLO") {
            scmd.sendmsg("HELLO");
        }
        else {
            scmd.sendmsg("ERR: unknown cmd");
        }
    }
}

void Server::getfile(Session& scmd) {
    std::string target;
    scmd.readline(target);
    target = getpath(config.path, target);
    if(!fs::exists(target)) {
        scmd.sendmsg("ERR: path not exisit");
        return ;
    }
    if(!fs::is_regular_file(target)) {
        scmd.sendmsg("ERR: it is not a file");
        return ;
    } 
    std::ifstream fin;
    fin.open(target);
    if(!fin) {
        scmd.sendmsg("ERR: can not open file");
        fin.close();
        return ;
    }
    scmd.sendmsg("OK");
    Session datasession = buildstream(scmd);
    if(!datasession.status()) {
        scmd.sendmsg("ERR: Can't build");
        return ;
    }
    scmd.sendmsg("BEGIN");
    datasession.sendstream(fin);
    fin.close();
    datasession.close();
}

void Server::list(Session& scmd) {
    std::string path;
    scmd.gettok(path);

    std::stringstream listinfo;
    if(getlist(getpath(config.path, path), listinfo)) {
        scmd.sendmsg("OK");
    } else {
        scmd.sendmsg("ERR: Path not exisit");
        return ;
    }
    Session datasession = buildstream(scmd);
    if(!datasession.status()) {
        scmd.sendmsg("ERR: Can't build");
        return ;
    }
    scmd.sendmsg("BEGIN");
    datasession.sendstream(listinfo, listinfo.tellp());
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
        Session session = Session::buildsession(target, local, PASSIVE);
        if(!session.status()) {
            logger("ERR: can not build data connection", "buildstream");
        }
        return std::move(session);

    } else if(cmd == "PASV") {
        Ipaddr local = scmd.getlocaladdr();
        local.port = 0;
        Session session = Session::buildlocalsession(local, CLOSE);

        if(!session.status()) {
            logger("ERR: can not build data connection", "buildstream");
            return std::move(session);
        }
        
        session.listen(1);
        scmd.sendmsg("PORT " + std::to_string(session.getlocaladdr().port));

        Session dsession = session.accept(5, PASSIVE);
        if(!dsession.status()) {
            logger("ERR: data connection time out", "buildstream");
        }
        // 建立会话
        return std::move(dsession);
    } else {
        logger("ERR: do not know which way");
        return Session::closedsession();
    }
}

bool Server::login(Session& scmd) {
    std::string cmd;
    scmd.sendmsg("LOGIN");

    // USER
    if(!scmd.expect("USER")) {
        scmd.sendmsg("ERR: expect USER");
        scmd.close();
        return false;
    }
    scmd.gettok(cmd);
    if(cmd == "anonymous" && !config.allowAnonymous){
        scmd.sendmsg("ERR: anonymous is not allow");
        scmd.close();
        return false;
    }
    if(cmd != "anonymous" && !config.users.count(cmd)) {
        scmd.sendmsg("ERR: user not exist");
        scmd.close();
        return false;
    }
    user = cmd;
    scmd.sendmsg("OK");
    
    // PASSWORD 
    if(!scmd.expect("PASSWORD")) {
        scmd.sendmsg("ERR: expect PASSWORD");
        scmd.close();
        return false;
    }
    scmd.gettok(cmd);
    if(user != "anonymous" && config.users.at(user) != cmd)  {
        scmd.sendmsg("ERR: authentication failed");
        scmd.close();
        return false;
    }
    passwd = cmd;
    scmd.sendmsg("OK");

    // DONE
    return true;
}


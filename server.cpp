#include "session.h"
#include "server.h"
#include "json.hpp"
#include <fstream>
#include <dirent.h>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

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


int ServerConfig::loadconfig(std::string filename) {
    std::ifstream fin;
    fin.open(filename);
    if(!fin) {
        std::cerr << "[Server Config] can't not open config file!" << std:: endl;
        return -1;
    }
    json info;
    try {
        fin >> info;
    } catch(nlohmann::detail::parse_error e) {
        std::cerr << "[Server Config] config parse error!" << std:: endl;
        return -1;
    }
    std::string loadedpath;
    std::map<std::string, std::string> loadedusers;
    
    if(info.contains("path")) {
        loadedpath = info["path"];
    } else {
        std::cerr << "[Server Config] lack path!" << std:: endl;
        return -1;
    }

    if(info.contains("users")) {
        for(auto& user : info["users"].items()) {
            loadedusers[user.key()] = user.value();
        }
    } else {
        std::cerr << "[Server Config] lack users!" << std:: endl;
        return -1;
    }

    if(info.contains("allow anonymous")) {
        allowAnonymous = info["allow anonymous"];
    } else {
        std::cerr << "[Server Config] anonymous?" << std:: endl;
        return -1;
    }

    path = loadedpath;
    users = loadedusers;
    return 0;
}


void Server::operator()(Session& scmd) {
    // init
    if(login(scmd)) return ;

    std::string cmd;
    while(1) {
        scmd.prereadcmd();
        scmd.gettok(cmd);

        if(cmd == "BYE") {
            scmd.sendmsg("BYE");
            break;
        } else {
            scmd.sendmsg("ERR: unknown cmd");
        }
    }
}

ServerConfig Server::config; 

int Server::login(Session& scmd) {
    std::string cmd;
    scmd.sendmsg("LOGIN");
    // USER
    scmd.readcmd();
    scmd.gettok(cmd);
    if(cmd != "USER") 
        cmderror("ERR: request error");
    
    scmd.gettok(cmd);
    if(cmd == "anonymous") {
        if(!config.allowAnonymous) 
            cmderror("ERR: anonymous is not allow");
    } else if(!config.users.count(cmd)) {
        cmderror("ERR: user not exist!");
    }
    user = cmd;
    scmd.sendmsg("OK");
    
    
    // PASSWORD 
    scmd.readcmd();
    scmd.gettok(cmd);
    if(cmd != "PASSWORD") 
        cmderror("ERR: request error");
    scmd.gettok(cmd);
    if(user != "anonymous" && config.users.at(user) != cmd) 
        cmderror("ERR: authentication failed");
    passwd = cmd;
    scmd.sendmsg("OK");

    // DONE
    return 0;
}


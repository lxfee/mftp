#include "server.h"
#include "json.hpp"
#include <fstream>
#include <dirent.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include "logger.hpp"
#include "utils.h"
#include "socket.h"


/***********************server config****************************/
#define ACPTTIMEOUT 10 // 建立数据连接时accept超时时间（秒）
using json = nlohmann::json;
namespace fs = std::filesystem;

ServerConfig::ServerConfig() {
    std::ifstream fin;
    fs::create_directories("tmp");
    fin.open("./config.json");
    if(!fin) {
        fin.close();
        std::ofstream fout;
        fout.open("./config.json");
        if(!fout) {
            panic("[Server Config] can't not create config file!");
        }
        json config;
        config["path"] = "./tmp";
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
            putfile(scmd);
        }
        else if(cmd == "TIME") {
            scmd.sendmsg(getcurrenttime());
        } 
        else if(cmd == "HELLO") {
            scmd.sendmsg("HELLO");
        }
        else if(cmd == "RUN") {
            runcmd(scmd);
        }
        else {
            scmd.sendmsg("ERR: unknown cmd");
        }
    }
}


#define MUSTNOT(EXP, MSG, OP) \
    if(EXP) {\
        scmd.sendmsg(MSG); \
        OP; \
        return ; \
    }

#define MUST(EXP, MSG, OP) \
    if(!(EXP)) {\
        scmd.sendmsg(MSG); \
        OP; \
        return ; \
    }

void Server::putfile(Session& scmd) {
    std::string savepath;
    scmd.readline(savepath);
    logger(savepath);
    savepath = getpath(config.path, savepath);
    MUST(config.users.count(user), "ERR: forbiden", );
    MUSTNOT(fs::exists(savepath), "ERR: file exist", );
    
    std::ofstream fout;
    fout.open(savepath, std::ios::binary);
    MUST(fout, "ERR: can not create file", fout.close());

    scmd.sendmsg("OK");
    Session datasession = buildstream(scmd);
    MUST(datasession.status(), "ERR: Can't build", );
    scmd.sendmsg("BEGIN");
    datasession.recvstream(fout);
    fout.close();
    logger("file saved: " + savepath);
}

void Server::runcmd(Session& scmd) {
    MUST(config.users.count(user), "ERR: forbiden", );
    std::string cmd;
    scmd.readline(cmd);
    scmd.sendmsg("OK");
    std::string tcmd = "cd " + config.path + "&& " + cmd.c_str();
    int status = system(tcmd.c_str());
    scmd.sendmsg("STATUS: " + std::to_string(status));
}

void Server::getfile(Session& scmd) {
    std::string filepath;
    scmd.readline(filepath);
    filepath = getpath(config.path, filepath);
    MUST(fs::exists(filepath), "ERR: path not exisit",);
    MUST(fs::is_regular_file(filepath), "ERR: it is not a file",);

    std::ifstream fin;
    fin.open(filepath, std::ios::binary);
    MUST(fin, "ERR: can not open file", fin.close());
    scmd.sendmsg("OK");
    Session datasession = buildstream(scmd);
    MUST(datasession.status(), "ERR: Can't build", );
    scmd.sendmsg("BEGIN");
    // 获得文件大小
    fin.seekg(0, fin.end);
    int fsize = fin.tellg();
    fin.seekg(0, fin.beg);

    datasession.sendstream(fin, fsize);
    fin.close();

    // 服务器主动关闭
    if(fsize == 0)
        datasession.close();
}

void Server::list(Session& scmd) {
    std::string path;
    scmd.gettok(path);

    std::stringstream listinfo;
    MUST(getlist(getpath(config.path, path), listinfo), "ERR: Path not exisit", );
    scmd.sendmsg("OK");
    Session datasession = buildstream(scmd);
    MUST(datasession.status(),"ERR: Can't build",);
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
            logger("ERR: can not build data connection", target.to_string());
        }
        return std::move(session);

    } else if(cmd == "PASV") {
        Ipaddr local = scmd.getlocaladdr();
        // local.port = 0;
        local.port += getrandom(20000, 30000);
        Session session = Session::buildlocalsession(local, CLOSE);

        if(!session.status()) {
            logger("ERR: can not build data connection", "buildstream");
            return std::move(session);
        }
        if(!session.listen(1)) {
            logger("ERR: can not build data connection", "buildstream");
            return std::move(session);
        }

        scmd.sendmsg("PORT " + std::to_string(session.getlocaladdr().port));

        Session dsession = session.accept(ACPTTIMEOUT, PASSIVE);
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


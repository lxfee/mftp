#include <iostream>
#include "socket.h"
#include "session.h"
#include <fstream>
#include <sstream>
#include "logger.hpp"
#include "utils.h"
#include "client.h"
#include <algorithm>
#include <iomanip>
#include <vector>


using namespace std;
#define CMDPORT  1234

namespace parse {
std::string buffer;
void gettok(string& cmd) {
    cmd.clear();
    while(!buffer.empty()) {
        if(isspace(buffer.back())) buffer.pop_back();
        else break;
    }
    while(!buffer.empty()) {
        if(!isspace(buffer.back())) {
            cmd.push_back(buffer.back());
            buffer.pop_back();
        }
        else break;
    }
}
void nextline() {
    getline(cin, buffer);
    reverse(buffer.begin(), buffer.end());
}
bool excpet(string exp) {
    nextline();
    string rel;
    gettok(rel);
    return exp == rel;
}
}

using namespace parse;

bool open(Session& scmd) {
    if(scmd.status()) {
        cout << "already connected" << endl;
        return false;
    } else {
        string ip;
        while(1) {
            gettok(ip);
            if(!ip.empty()) break;
            cout << "target [IP:PORT] : ";
            nextline();
        }
        Ipaddr target;
        if(!parseIp(target, ip)) {
            cout << "can not parse address" << endl;
            return false;
        }
        if(!scmd.starttargetsession(target, ACTIVE)) {
            cout << "can not locate target" << endl;
            return false;
        }
        if(!scmd.expect("LOGIN")) {
            cout << "unexpect situation" << endl;
            return false;
        }
        string user, passwd;
        cout << "USER: ";
        nextline();
        gettok(user);
        cout << "PASSWORD: ";
        nextline();
        gettok(passwd);
        if(login(scmd, user, passwd)) {
            cout << "open success" << endl;
            return true;
        } else {
            cout << "login failed" << endl;
            return false;
        }
    }
    return true;
}

void close(Session& scmd) {
    if(scmd.status()) {
        scmd.sendmsg("BYE");
        scmd.close();
    }
}

void sync(Session& scmd) {
    scmd.sendmsg("HELLO");
    while(!scmd.expect("HELLO"));
}

void status(Session& scmd) {
    if(scmd.status()) {
        sync(scmd);
        Ipaddr local = scmd.getlocaladdr();
        Ipaddr target = scmd.gettargetaddr();
        cout << "connecting" << endl;
        cout << "[local]  " << local.getaddr() << ":" << local.port << endl;
        cout << "[target] " << target.getaddr() << ":" << target.port << endl;
    } else {
        cout << "not connected" << endl;
    }
}



void list(Session& scmd) {
    if(!scmd.status()) {
        cout << "not connected" << endl;
    } else {
        string path;
        gettok(path);
        if(path.empty()) path = "."; 
        scmd.sendmsg("LIST " + path);
        if(!scmd.expect("OK")) {
            cout << "request refused" << endl;
            return ;
        }
        Session datasession = buildstream(scmd, ACTV);
        if(!scmd.expect("BEGIN")) {
            cout << "can not build data session" << endl;
            return ;
        }
        if(!datasession.status()) {
            cout << "error ocur!" << endl;
            return ;
        }
        stringstream listinfo;
        datasession.recvstream(cout);
        cout << endl;
    }
}

vector<string> cmds = {
    "ls"        , "列出目录下的文件和文件夹",
    "help"      , "帮助",
    "status"    , "查看连接状态",
    "open"      , "打开一个连接",
    "close"     , "关闭当前连接",
    "bye"       , "关闭客户端"
};

void printhelp() {
    string cmd;
    gettok(cmd);
    if(cmd.empty()) {
        cout << "输入help [cmd]查看帮助" << endl;
        for(int i = 0; i < cmds.size(); i += 2) {
            cout << setw(10) << cmds[i];
            if((i / 2 + 1) % 4 == 0) cout << endl;
        }
        cout << endl;
    } else {
        bool found = false;
        for(int i = 0; i < cmds.size(); i+= 2) {
            if(cmds[i] == cmd) {
                cout << cmd << ": " << cmds[i + 1] << endl;
                found = true;
                break;
            }
        }
        if(!found) cout << cmd << ": " << "找不到指令" << endl;
    }
}

bool prasecmd(Session& scmd, string cmd) {
    if(cmd == "open") {
        open(scmd);
    } 
    else if(cmd == "status") {
        status(scmd);
    }
    else if(cmd == "close") {
        close(scmd);    
    }
    else if(cmd == "bye") {
        close(scmd);
        return false;
    }
    else if(cmd == "ls") {
        list(scmd);
    }
    else if(cmd == "help") {
        printhelp();
    }
    return true;
}



int main() {
    // init
    string cmd;
    Session scmd = Session::closedsession();
    while(1) {
        cout << "mftp> ";
        nextline();
        gettok(cmd);
        if(cmd.empty()) continue;
        try {
            if(!prasecmd(scmd, cmd)) break;
        } catch(string msg) {
            cout << "Session closed" << endl;
        }
    }
}
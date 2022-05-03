#include <iostream>
#include "socket.h"
#include "session.h"
#include <fstream>
#include <sstream>
#include "logger.hpp"
#include "utils.h"
#include "client.h"
#include <algorithm>


using namespace std;
#define CMDPORT  1234
enum DCONNECTMODE {PASV, ACTV} mode;

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

void status(Session& scmd) {
    if(scmd.status()) {
        scmd.sendmsg("HELLO");
        if(!scmd.expect("HELLO")) {
            close(scmd);
            cout << "not connected" << endl;
            return ;
        }
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
    return true;
}



int main() {
    // init
    string cmd;
    Session scmd = Session::nullsession();
    mode = PASV;

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
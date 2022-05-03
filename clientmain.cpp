#include <iostream>
#include "session.h"
#include <sstream>
#include "logger.hpp"
#include "utils.h"
#include "client.h"
#include <vector>
#include <iomanip>
using namespace std;

extern CONNECTMODE mode;

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
    else if(cmd == "time") {
        gettime(scmd);
    }
    else if(cmd == "get") {
        getfile(scmd);
    }
    else if(cmd == "put") {
        putfile(scmd);
    }
    else if(cmd == "run") {
        runcmd(scmd);
    }
    return true;
}



int main() {
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
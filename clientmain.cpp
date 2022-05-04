#include <iostream>
#include "session.h"
#include <sstream>
#include "logger.hpp"
#include "utils.h"
#include "client.h"
#include <vector>
#include <iomanip>
#ifdef WINDOWS
#include <winsock2.h>
#endif
using namespace std;
bool debugflag = false;

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
    else if(cmd == "debug") {
        debugflag = debugflag ^ 1;
        cout << "debug: " << debugflag << endl;
    }
    else if(cmd == "!") {
        runlocalcmd();
    }
    else if(cmd == "mode") {
        if(mode == ACTV) {
            mode = PASV;
            cout << "PASV" << endl;
        } else {
            mode = ACTV;
            cout << "ACTV" << endl;
        }
    }
    else {
        cout << "unknown cmd" << endl;
    }
    return true;
}



int main() {
    #ifdef WINDOWS
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif

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
            logger(msg);
            logger("Session disconnected");
            cout << "Session disconnected" << endl;
        }
    }

    #ifdef WINDOWS
    WSACleanup();
    #endif
}
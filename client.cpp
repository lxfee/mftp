#include <iostream>
#include "socket.h"
#include "session.h"
#include <fstream>
#include <sstream>
#include "logger.hpp"

using namespace std;
#define CMDPORT  1234

int main() {
    Ipaddr servaddr("127.0.0.1", CMDPORT);

    Session scmd = Session::buildtargetsession(servaddr, ACTIVE);
    if(!scmd.status()) {
        logger("can not connect!");
        return 1;
    }

    string cmd;
    while(1) {
        scmd.nextline();
        scmd.gettok(cmd);
        getline(cin, cmd);
        scmd.sendmsg(cmd);
        if(cmd == "BYE") break;
    }
}
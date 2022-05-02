#include <iostream>
#include "socket.h"
#include "session.h"
#include <fstream>
#include <sstream>
#include "logger.hpp"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


using namespace std;
#define CMDPORT  12343


Session buildstream(Session& scmd) {
    std::string cmd;
    
    scmd.nextline();
    scmd.gettok(cmd);

    if(cmd == "PORT") {
        scmd.gettok(cmd);
        Ipaddr target = scmd.gettargetaddr();
        target.port = atoi(cmd.c_str()); 
        
        Ipaddr local = scmd.getlocaladdr();
        local.port = 0;

        Session session = Session::buildsession(target, local, PASSIVE);
        return std::move(session);

    } else {//if(cmd == "PASV") {
        Ipaddr local = scmd.getlocaladdr();
        local.port = 0;
        Session session = Session::buildlocalsession(local, CLOSE);

        if(!session.status()) {
            return std::move(session);
        }
        
        session.listen(1);
        scmd.sendmsg("PORT " + std::to_string(session.getlocaladdr().port));
        logger("waiting");
        Session dsession = session.accept(1, PASSIVE);
        logger("done");
        if(dsession.status() < 0) {
        }
        // 建立会话
        return std::move(dsession);

    } 
    // else {
    //     return Session::nullsession();
    // }
}


int main() {
    Ipaddr servaddr("127.0.0.1", CMDPORT);
    Session scmd = Session::buildtargetsession(servaddr, ACTIVE);
    if(!scmd.status()) {
        logger("can not connect!");
        return 1;
    }

    string cmd;
    while(1) {
        
        getline(cin, cmd);
        scmd.sendmsg(cmd);
        if(cmd == "BYE") break;
        if(cmd == "LIST") {
            Session d = buildstream(scmd);
            if(!d.status()) {
                logger("build data session error", "ERROR");
                continue;
            }
            string s;
            stringstream ss(s);
            d.recvstream(ss);
            string res;
            while(ss >> res) cout << res << endl;
            scmd.nextline();
        }
        scmd.nextline();
        scmd.gettok(cmd);
    }
}
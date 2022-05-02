#include <iostream>
#include "socket.h"
#include "session.h"
#include <fstream>
#include <sstream>
#include "logger.hpp"

using namespace std;
#define CMDPORT  12345


SessionPtr buildStreamTp(Session& scmd) {
    std::string cmd;
    scmd.readcmd();
    scmd.gettok(cmd);
    if(cmd == "PORT") {
        scmd.gettok(cmd);
        logger(cmd);
        Ipaddr clientaddr = scmd.addr;
        clientaddr.port = atoi(cmd.c_str()); 
        logger("he");
        Socket servsock;
        if(servsock.connect(clientaddr) < 0) {
            scmd.sendmsg("ERR: can not build data connection");
            return nullptr;
        }
        // 建立会话
        return std::move(std::make_unique<Session>(clientaddr ,servsock, PASSIVE));
    } else if(cmd == "PASV") {
        Socket servsock;
        Ipaddr saddr;
        logger(scmd.sock.getsockname(saddr));
        saddr.port = 0;
        if(servsock.bind(saddr) < 0) {
            scmd.sendmsg("ERR: can not build data connection");
            return nullptr;
        }
        logger(scmd.sock.getsockname(saddr));
        servsock.listen(1);
        scmd.sendmsg("PORT " + std::to_string(saddr.port));
        Ipaddr clientaddr;
        Socket clientsock;
        servsock.setrecvtimeout(1);
        if(servsock.accept(clientaddr, clientsock) < 0) {
            scmd.sendmsg("ERR: data connection time out");
            return nullptr;
        }
        servsock.close();
        
        // 建立会话
        return std::move(std::make_unique<Session>(clientaddr ,clientsock, PASSIVE));
    } 
    // else {
    //     scmd.sendmsg("ERR: do not know which way");
    //     return nullptr;
    // }
}

int main() {
    Socket clientsock;
    Ipaddr servaddr("127.0.0.1", CMDPORT);
    clientsock.connect(servaddr);
    string cmd;
    Session scmd(servaddr, clientsock, CLOSEMODE::ACTIVE);
    while(1) {
        scmd.readcmd();
        scmd.gettok(cmd);
        getline(cin, cmd);
        scmd.sendmsg(cmd);
        if(cmd == "BYE") break;
        if(cmd == "PASV") {
            auto datass = buildStreamTp(scmd);
            scmd.readcmd();
            string tmp;
            stringstream ss(tmp);
            datass->recvstream(ss);
            string res;
            while(ss >> res) {
                cout << res << endl;
            }
        }
        
    }
}
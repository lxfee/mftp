#include <iostream>
#include "socket.h"
#include "session.h"
#include <fstream>

using namespace std;
#define CMDPORT  1141
#define DATAPORT 514

int main() {
    Socket clientsock;
    Ipaddr clientaddr("127.0.0.1", CMDPORT);
    clientsock.connect(clientaddr);
    string cmd;
    Session scmd(clientaddr, clientsock, CLOSEMODE::ACTIVE);
    scmd.recvmsg(cmd);
    while(1) {
        if(cmd == "BYE") break;
        getline(cin, cmd);
        scmd.sendmsg(cmd);
    }
}
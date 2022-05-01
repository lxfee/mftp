#include <iostream>
#include "socket.h"
#include "session.h"
#include <fstream>

using namespace std;

int main() {
    Socket clientsock;
    Ipaddr clientaddr("127.0.0.1", 1234);
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
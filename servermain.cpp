#include <iostream>
#include <server.h>
#include <thread>
#include <mutex>
#include "logger.hpp"
using namespace std;

#define MAX_CONNECTIONS 2

mutex mutx;
int runningthread = 0;

bool incthread() {
    bool flag = false;
    mutx.lock();
    if(runningthread < MAX_CONNECTIONS) {
        flag = true;
        runningthread++;
    }
    mutx.unlock();
    return flag;
}

void decthread() {
    mutx.lock();
    runningthread--;
    mutx.unlock();
}

void serverthread(Ipaddr addr, Socket sock) {
    Session scmd(addr, sock, CLOSEMODE::PASSIVE);
    if(!incthread()) {
        scmd.sendmsg("ERR: too many connections");
        return ;
    } else {
        Server server;
        try {
            server(scmd);
        } catch(const char* msg) {
            logger("Session disconnected");
        }
        decthread();
    }
}


int main() {
    Socket servsock;
    if(servsock.bind(Server::config.addr) < 0) {
        logger("Start failed");
        return 1;
    } 
    servsock.listen(MAX_CONNECTIONS);
    
    logger("Server listening on 0.0.0.0", Server::config.addr.port);
    
    Ipaddr clientaddr;
    Socket clientsock;

    while(1) {
        if(servsock.accept(clientaddr, clientsock) < 0) break;
        thread th(serverthread, clientaddr, clientsock);
        th.detach();
    }
    
    servsock.close();
}
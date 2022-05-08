#include <iostream>
#include <server.h>
#include <thread>
#include <mutex>
#include <ctime>
#include "logger.hpp"
#include<unistd.h>
#ifdef WINDOWS
#include <winsock2.h>
#endif
using namespace std;
bool debugflag = true;

#define MAX_CONNECTIONS 5

mutex mutx;
int runningthread = 0;

void printthread() {
    while(1) {
        sleep(10);
        logger("running thread: " + to_string(runningthread), "Thread Printer");
    }
}

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

void serverthread(Session scmd) {
    if(!incthread()) {
        scmd.sendmsg("ERR: too many connections");
        return ;
    } else {
        Server server;
        try {
            server(scmd);
        } catch(string msg) {
            logger(msg);
            logger("Session disconnected");
        }
        decthread();
    }
}


int main() {
    #ifdef WINDOWS
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif

    Session localsession = Session::buildlocalsession(Server::config.addr, CLOSE);
    
    if(!localsession.status()) {
        logger("Start failed");
        return 1;
    } 
    localsession.listen(MAX_CONNECTIONS);
    logger("Server listening on " + localsession.getlocaladdr().to_string(), localsession.getlocaladdr().to_string());
    

    thread tmpth(printthread);
    tmpth.detach();

    while(1) {
        Session clientsession = localsession.accept(PASSIVE);
        if(!clientsession.status()) break;
        thread th(serverthread, std::move(clientsession));
        // 必须要等待子线程函数准备好才继续
        // 否则clientsession循环后被提前析构，导致子线程无法构造出错，内存泄露
        while(!th.joinable());
        th.detach();
    }

    #ifdef WINDOWS
    WSACleanup();
    #endif
}
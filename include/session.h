#pragma once
#include "socket.h"
#include <string>
#include <memory>
#define BUFFER_SIZE 512
enum CLOSEMODE {ACTIVE, PASSIVE, NOTCLOSE};

class Session {
public:
    static Session buildtargetsession(Ipaddr target, CLOSEMODE mode);
    static Session buildlocalsession(Ipaddr local, CLOSEMODE mode);
    static Session buildsession(Ipaddr target, Ipaddr local, CLOSEMODE mode);
    
    int bind(Ipaddr local);
    int listen(int backlog);
    Session accept(int sec = 0);
    int getlocalport();

    Session(const Session &) = delete;
    Session(Session &&);
    ~Session();
    

    void sendmsg(const std::string& msg);
    void sendstream(std::istream& os);
    void recvmsg(std::string& smg);
    void recvstream(std::ostream& is);

    void gettok(std::string& cmd);
    void nextline();

    bool status();
    
private:
    Session();
    bool eoftag; // 给gettok准备
    int wait();
    CLOSEMODE mode;
    Socket sock;
    Ipaddr target;
    Ipaddr local;
    int buildstatus;
};

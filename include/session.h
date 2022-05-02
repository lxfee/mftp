#pragma once
#include "socket.h"
#include <string>
#include <memory>
#define BUFFER_SIZE 512
enum CLOSEMODE {ACTIVE, PASSIVE, CLOSE, NOTCLOSE};

class Session {
public:
    static Session buildtargetsession(Ipaddr target, CLOSEMODE mode);
    static Session buildlocalsession(Ipaddr local, CLOSEMODE mode);
    static Session buildsession(Ipaddr target, Ipaddr local, CLOSEMODE mode);
    static Session nullsession();

    int bind(Ipaddr local);
    int listen(int backlog);
    Session accept(int sec = 0, CLOSEMODE mode = PASSIVE);
    Ipaddr getlocaladdr();
    Ipaddr gettargetaddr();

    Session(const Session &) = delete;
    Session(Session &&) noexcept ;
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
    int wait();
    CLOSEMODE mode;
    Socket sock;
    Ipaddr target;
    Ipaddr local;
    std::string buffer;
    int buildstatus;
};

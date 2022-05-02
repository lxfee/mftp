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

    bool starttargetsession(Ipaddr target, CLOSEMODE mode);
    bool startlocalsession(Ipaddr local, CLOSEMODE mode);
    bool startsession(Ipaddr target, Ipaddr local, CLOSEMODE mode);

    

    int bind(Ipaddr local);
    int listen(int backlog);
    void close();
    Session accept(int sec = 0, CLOSEMODE mode = PASSIVE);
    Ipaddr getlocaladdr();
    Ipaddr gettargetaddr();

    Session(const Session &) = delete;
    Session(Session &&) noexcept ;
    ~Session();
    

    void sendmsg(const std::string& msg);
    void recvmsg(std::string& smg);
    void sendstream(std::istream& os, int size = 0);
    void recvstream(std::ostream& is);

    void gettok(std::string& cmd);
    void readline(std::string& cmd);
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

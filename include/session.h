#pragma once
#include "socket.h"
#include <string>
#include <memory>
#define BUFFER_SIZE 512
enum STATUS {ACTIVE = 0, PASSIVE = 1, CLOSE = 2, CLOSED = 3};

class Session {
public:
    // 涉及创建新session的方法
    static Session closedsession();
    static Session buildtargetsession(Ipaddr target, STATUS mode);
    static Session buildlocalsession(Ipaddr local, STATUS mode);
    static Session buildsession(Ipaddr target, Ipaddr local, STATUS mode);
    
    bool starttargetsession(Ipaddr target, STATUS mode);
    bool startlocalsession(Ipaddr local, STATUS mode);
    bool startsession(Ipaddr target, Ipaddr local, STATUS mode);

    Session accept(int sec = 0, STATUS mode = PASSIVE);

    

    
    // 涉及socket的方法
    bool listen(int backlog);
    void close();

    // getter
    Ipaddr getlocaladdr();
    Ipaddr gettargetaddr();
    bool status();

    // 初始化
    Session(const Session &) = delete;
    Session(Session &&) noexcept ;
    ~Session();
    
    // 异常
    void throwerror(std::string msg);

    // 方便读取指令的方法
    void gettok(std::string& cmd);
    void readline(std::string& cmd);
    void nextline();
    bool expect(std::string exp);

    // 基础交流方法
    void sendmsg(const std::string& msg);
    void recvmsg(std::string& smg);
    void sendstream(std::istream& os, int size = 0);
    void recvstream(std::ostream& is);

    
private:
    bool renew();
    Session();
    Session(const Socket& sock);
    int wait();
    STATUS sstatus;
    Socket sock;
    Ipaddr target;
    Ipaddr local;
    std::string buffer;
};

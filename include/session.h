#pragma once
#include "socket.h"
#include <string>
#define BUFFER_SIZE 512
enum CLOSEMODE {ACTIVE, PASSIVE};

class Session {
public:
    Session(const Ipaddr& addr, const Socket& sock, CLOSEMODE mode);
    Session(const Session &) = delete;
    ~Session();
    
    void sendmsg(const std::string& msg);
    void sendstream(std::istream& os);
    void recvmsg(std::string& smg);
    void recvstream(std::ostream& is);

    void gettok(std::string& cmd);
    void readcmd();
    void prereadcmd();
private:
    int wait();
    Socket sock;
    Ipaddr addr;
    std::string buffercmd;
    std::string prebuffercmd;
    CLOSEMODE mode;
};



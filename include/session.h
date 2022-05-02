#pragma once
#include "socket.h"
#include <string>
#include <memory>
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
    Socket sock;
    Ipaddr addr;
    
private:
    int wait();
    std::string buffercmd;
    CLOSEMODE mode;
};

using SessionPtr = std::unique_ptr<Session>;


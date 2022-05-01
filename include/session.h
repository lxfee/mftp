#pragma once
#include "socket.h"
#include <string>

#define BUFFER_SIZE 512
enum CLOSEMODE {ACTIVE, PASSIVE};
class Session {
public:
    Session(const Ipaddr& addr, const Socket& sock, CLOSEMODE mode);
    ~Session();
    int sendmsg(const std::string& msg);
    int sendstream(std::istream& os);
    int recvmsg(std::string& smg);
    int recvstream(std::ostream& is);
    void gettok(std::string& cmd);
    int wait();
    int readcmd();
    int prereadcmd();


private:
    Socket sock;
    Ipaddr addr;
    std::string buffercmd;
    std::string prebuffercmd;
    CLOSEMODE mode;
};



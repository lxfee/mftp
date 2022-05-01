#include "session.h"
#include <iostream>
#include <algorithm>

Session::Session(const Ipaddr& addr, const Socket& sock, CLOSEMODE mode) : addr(addr), sock(sock), mode(mode) {}

int Session::wait() {
    char buffer[BUFFER_SIZE];
    int nbytes;
    while((nbytes = sock.read(buffer, sizeof buffer)) > 0);
    return nbytes;
}

Session::~Session() {
    if(mode == CLOSEMODE::PASSIVE) {
        wait();
        sock.shutdown(SD_WR);
    } else {
        sock.shutdown(SD_WR);
        wait();
    }
    sock.close();
}

int Session::sendmsg(const std::string& msg) {
    std::cout << "SEND: " << msg << std::endl;

    std::string tmsg = msg;
    if(tmsg.back() != '\n') tmsg.push_back('\n'); // 换行符为结尾
    return sock.write(tmsg.c_str(), tmsg.size());
}

int Session::sendstream(std::istream& is) {
    char buffer[BUFFER_SIZE];
    int nbytes;
    int totbytes = 0;
    do {
        is.read(buffer, sizeof(buffer));
        nbytes = is.gcount();
        if(sock.write(buffer, nbytes) < 0) break; 
        totbytes += nbytes;
    } while(nbytes);
    return totbytes;
}

int Session::recvmsg(std::string& msg) {
    std::cout << "RECV: ";

    msg.clear();
    char ch;
    int nbytes;
    int totbytes = 0;
    while((nbytes = sock.read(&ch, 1)) > 0) {
        totbytes++;
        if(ch == '\n') break;
        msg.push_back(ch);
    }
    std::cout << msg << std::endl;
    if(nbytes < 0) return -1;
    
    return totbytes;
}

int Session::recvstream(std::ostream& os) {
    char buffer[BUFFER_SIZE];
    int nbytes;
    int totbytes = 0;
    while((nbytes = sock.read(buffer, sizeof(buffer))) > 0) {
        totbytes += nbytes;
        os.write(buffer, nbytes);
    }
    return totbytes;
}

void Session::gettok(std::string& cmd) {
    cmd.clear();
    while(!buffercmd.empty()) {
        if(isspace(buffercmd.back())) buffercmd.pop_back();
        else break;
    }
    while(!buffercmd.empty()) {
        if(isspace(buffercmd.back())) break;
        cmd.push_back(buffercmd.back());
        buffercmd.pop_back();
    }
}


int Session::prereadcmd() {
    int nbytes = 0;
    nbytes = recvmsg(prebuffercmd);
    buffercmd = prebuffercmd;
    std::reverse(buffercmd.begin(), buffercmd.end());
    return nbytes;
}

int Session::readcmd() {
    if(!prebuffercmd.empty()) {
        swap(buffercmd, prebuffercmd);
        prebuffercmd.clear();
        std::reverse(buffercmd.begin(), buffercmd.end());
        return buffercmd.size();
    }
    int nbytes = recvmsg(buffercmd);
    std::reverse(buffercmd.begin(), buffercmd.end());
    return nbytes;
}
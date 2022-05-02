#include "session.h"
#include "logger.hpp"
#include <algorithm>
#include <exception>
#include <iostream>

#define error(msg) {buildstatus = -1; throw msg;}

int Session::wait() {
    int nbytes;
    char ch;
    while((nbytes = sock.read(&ch, 1)) > 0);
    return nbytes;
}
Session::Session() : mode(NOTCLOSE), buildstatus(-1) {}

Session::Session(Session && session) noexcept :
    buffer(std::move(session.buffer)), 
    local(session.local),
    target(session.target), 
    sock(session.sock),
    mode(session.mode),
    buildstatus(session.buildstatus)
{
    session.mode = NOTCLOSE;
    session.buildstatus = -1;
}

Session::~Session() {
    switch (mode) {
        case PASSIVE:
            logger("Wait session closed", target.port);
            wait();
            sock.shutdown(SD_WR);
            break;
        case ACTIVE:
            sock.shutdown(SD_WR);
            wait();
            break;
        case CLOSE:
            break;
        case NOTCLOSE:
        default:
            return ;
    }
    sock.close();
    logger("Session closed", target.port);
}

void Session::close() {
    switch (mode) {
        case CLOSE:
            break;
        case NOTCLOSE:
            return ;
        default:
            sock.shutdown(SD_WR);
            wait();
            break;
    }
    sock.close();
    mode = NOTCLOSE;
    buildstatus = -1;
    logger("Session closed", target.port);
}

void Session::sendmsg(const std::string& msg) {
    logger("SEND: " + msg, target.port);
    std::string tmsg = msg;
    if(tmsg.back() != '\n') tmsg.push_back('\n'); // 换行符为结尾
    if(sock.write(tmsg.c_str(), tmsg.size()) < 0) {
        error("session closed");
    }
}


// size = 0代表保持发送，直到会话被关闭
void Session::sendstream(std::istream& is, int size) {
    // 发送待发送的字节数
    sock.write(&size, 4);
    char buffer[BUFFER_SIZE];
    

    if(size == 0) {
        int nbytes = sizeof(buffer);
        // 发到流发完为止
        do {
            is.read(buffer, sizeof(buffer));
            nbytes = is.gcount();
            if(sock.write(buffer, nbytes) < 0) {
                error("session closed");
            }
        } while(nbytes);
    } else {
        int nbytes = std::min((int)sizeof(buffer), size);
        // 发送特定数量字节
        do {
            is.read(buffer, nbytes);
            nbytes = is.gcount();
            if(sock.write(buffer, nbytes) < 0) {
                error("session closed");
            }
            size -= nbytes;
            nbytes = std::min(size, nbytes);
        } while(nbytes);
    }
}


void Session::recvmsg(std::string& msg) {
    msg.clear();
    char ch;
    int nbytes;
    while((nbytes = sock.read(&ch, 1)) > 0) {
        if(ch == '\n') break;
        msg.push_back(ch);
    }
    logger("RECV: " + msg, target.port);
    if(nbytes <= 0) {
        error("session closed");
    }
}

void Session::recvstream(std::ostream& os) {
    // 接收字节数
    int size;
    sock.read(&size, 4);
    
    // 接收数据
    char buffer[BUFFER_SIZE];
    int nbytes = std::min(size, (int)sizeof(buffer));

    
    if(size == 0) {
        // 未指明数据大小
        while((nbytes = sock.read(buffer, sizeof(buffer))) > 0) {
            os.write(buffer, nbytes);
        }
    } else {
        // 接收特定数量字节
        while((nbytes = sock.read(buffer, nbytes)) > 0) {
            os.write(buffer, nbytes);
            size -= nbytes;
            nbytes = std::min(size, (int)sizeof(buffer));
            if(!nbytes) break;
        }
    }
    if(nbytes < 0) {
        error("session closed");
    }
}

void Session::gettok(std::string& cmd) {
    cmd.clear();
    while(!buffer.empty()) {
        if(isspace(buffer.back())) buffer.pop_back();
        else break;
    }
    while(!buffer.empty()) {
        if(!isspace(buffer.back())) {
            cmd.push_back(buffer.back());
            buffer.pop_back();
        }
        else break;
    }
}

void Session::readline(std::string& cmd) {
    cmd = buffer;
    std::reverse(cmd.begin(), cmd.end());
}

void Session::nextline() {
    recvmsg(buffer);
    std::reverse(buffer.begin(), buffer.end());
}

// 判断会话是否有效
bool Session::status() {
    if(buildstatus < 0) return false;
    return true;
}

Session Session::nullsession() {
    return std::move(Session());
}

bool Session::starttargetsession(Ipaddr target, CLOSEMODE mode) {
    if(status()) return false;
    int flag = sock.connect(target);
    buildstatus = flag;
    if(flag < 0) {
        return false;
    }
    this->mode = mode;
    this->target = target;
    sock.getsockname(this->local);
    return true;
}

bool Session::startlocalsession(Ipaddr local, CLOSEMODE mode) {
    if(status()) return false;
    int flag = sock.bind(local);
    buildstatus = flag;
    if(flag < 0) {
        return false;
    }
    this->mode = mode;
    sock.getsockname(this->local);
    return true;
}

bool Session::startsession(Ipaddr target, Ipaddr local, CLOSEMODE mode) {
    if(status()) return false;
    int flag = sock.bind(local);
    if(flag < 0) {
        return false;
    }
    flag = sock.connect(target);
    buildstatus = flag;
    if(flag < 0) {
        return false;
    }
    this->mode = mode;
    this->target = target;
    sock.getsockname(this->local);
    return true;
}


Session Session::buildtargetsession(Ipaddr target, CLOSEMODE mode) {
    Session session;
    session.starttargetsession(target, mode);
    return std::move(session);
}

Session Session::buildlocalsession(Ipaddr local, CLOSEMODE mode) {
    Session session;
    session.startlocalsession(local, mode);
    return std::move(session);
}


Session Session::buildsession(Ipaddr target, Ipaddr local, CLOSEMODE mode) {
    Session session;
    session.startsession(target, local, mode);
    return std::move(session);
}



int Session::bind(Ipaddr local) {
    int flag = sock.bind(local);
    if(flag < 0) {
        return flag;
    }
    sock.getsockname(this->local);
    return flag;
}

int Session::listen(int backlog) {
    return sock.listen(backlog);
}

Session Session::accept(int sec, CLOSEMODE mode) {
    if(sec) {
        sock.setrecvtimeout(sec);
    }
    Ipaddr target;
    Socket nsock;
    Session session;
    int flag = sock.accept(target, nsock);
    session.buildstatus = flag;
    if(flag < 0) {
        return std::move(session);
    } 
    nsock.setrecvtimeout(0);
    session.sock = nsock;
    session.target = target;
    nsock.getsockname(session.local);
    session.mode = mode;
    return std::move(session);
}

Ipaddr Session::getlocaladdr() {
    return local;
}

Ipaddr Session::gettargetaddr() {
    return target;
}

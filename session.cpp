#include "session.h"
#include "logger.hpp"
#include <algorithm>
#include <exception>
#include <iostream>
#include <cassert>

void Session::throwerror(std::string msg) {
    msg = "[error]" + msg;
    if(sstatus != CLOSED) sstatus = CLOSE;
    close();
    throw msg;
}

int Session::wait() {
    int nbytes;
    char ch;
    // 直到返回0（对方关闭），或返回-1（出错）
    while((nbytes = sock.read(&ch, 1)) > 0);
    return nbytes;
}

Session::Session() : sstatus(CLOSE) {}
Session::Session(const Socket& sock) : sock(sock), sstatus(CLOSE) {}

Session::Session(Session && session) noexcept :
    buffer(std::move(session.buffer)), 
    local(session.local),
    target(session.target), 
    sock(session.sock),
    sstatus(session.sstatus)
{
    session.sstatus = CLOSED;
}

Session::~Session() {
    switch (sstatus) {
        case PASSIVE:
            // 被动模式
            logger("Wait session closed", target.to_string());
            wait();
            sock.shutdown(SD_WR);
            break;
        case ACTIVE:
            // 主动模式
            sock.shutdown(SD_WR);
            wait();
            break;
        case CLOSE:
            break;
        case CLOSED:
        default:
            return ;
    }
    // shutdown只是关闭连接，close才是操作系统真正回收套接字资源
    sock.close();
    logger("Session closed", target.to_string());
}

void Session::close() {
    switch (sstatus) {
        case CLOSED:
            return ;
        case CLOSE:
            break;
        default:
            sock.shutdown(SD_WR);
            wait();
            break;
    }
    sock.close();
    sstatus = CLOSED;
    logger("Session closed", target.to_string());
}

void Session::sendmsg(const std::string& msg) {
    logger("SEND: " + msg, target.to_string());
    if(sock.write(msg.c_str(), msg.size()) < 0) {
        throwerror("session closed");
    }
    // 换行符为结尾
    if(msg.back() != '\n') {
        if(sock.write("\n", 1) < 0) {
            throwerror("session closed");
        }        
    }

}

// size = -1代表保持发送，直到会话被关闭
void Session::sendstream(std::istream& is, size_t size) {
    // 发送待发送的字节数
    sock.write(&size, sizeof(size));
    logger(size, "send size");
    char buffer[BUFFER_SIZE];
    size_t tot = size;
    size_t nbytes;
    if(size == -1) {
        nbytes = sizeof(buffer);
        // 发到流发完为止
        do {
            is.read(buffer, sizeof(buffer));
            nbytes = is.gcount();
            logger(nbytes, "send - from file");
            if(nbytes <= 0) break;
            if((nbytes = sock.write(buffer, nbytes)) < 0) {
                throwerror("session closed");
            }
            logger(nbytes, "send - to socket");
        } while(nbytes);
    } else {
        if(size == 0) return ;
        nbytes = std::min(size, sizeof(buffer));
        // 发送特定数量字节
        do {
            is.read(buffer, nbytes);
            nbytes = is.gcount();
            if(nbytes <= 0) break;
            // logger(nbytes, "send - from file");
            if((nbytes = sock.write(buffer, nbytes)) < 0) {
                throwerror("session closed");
            }
            // logger(nbytes, "send - to socket");
            size -= nbytes;
            nbytes = std::min(size, nbytes);
            printprocess(tot - size, tot, "sending", target.to_string());
            assert(nbytes >= 0);
        } while(nbytes);
    }
    if(nbytes < 0) {
        throwerror("session closed");
    }
}

void Session::recvmsg(std::string& msg) {
    msg.clear();
    char ch;
    int nbytes;
    while((nbytes = sock.read(&ch, 1)) > 0) {
        // 一个字符一个字符地读，直到读到换行符为止
        if(ch == '\n') break;
        msg.push_back(ch);
    }
    logger("RECV: " + msg, target.to_string());
    if(nbytes <= 0) {
        throwerror("session closed");
    }
}

void Session::recvstream(std::ostream& os) {
    // 接收字节数
    size_t size;
    sock.read(&size, sizeof(size));
    size_t tot = size;
    logger(size, "recv size");
    // 接收数据
    char buffer[BUFFER_SIZE];
    size_t nbytes = std::min(size, sizeof(buffer));
    if(size == -1) {
        // 未指明数据大小
        while((nbytes = sock.read(buffer, sizeof(buffer))) > 0) {
            logger(nbytes, "recv - from socket");
            os.write(buffer, nbytes);
        }
    } else {
        // 接收特定数量字节
        if(!size) return ;
        while((nbytes = sock.read(buffer, nbytes)) > 0) {
            // logger(nbytes, "recv - from socket");
            os.write(buffer, nbytes);
            size -= nbytes;
            nbytes = std::min(size, sizeof(buffer));

            printprocess(tot - size, tot, "recving", target.to_string());
            if(!nbytes) break;
            assert(nbytes >= 0);
        }
    }
    if(nbytes < 0) {
        throwerror("session closed");
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

// read a line, check if first token is exp
bool Session::expect(std::string exp) {
    nextline();
    std::string rel;
    gettok(rel);
    return exp == rel;
}

void Session::readline(std::string& line) {
    line = buffer;
    while(isspace(line.back())) line.pop_back();
    reverse(line.begin(), line.end());
}

void Session::nextline() {
    recvmsg(buffer);
    std::reverse(buffer.begin(), buffer.end());
}

// 判断会话是否有效
bool Session::status() {
    return sstatus != CLOSED;
}

bool Session::renew() {
    switch (sstatus) {
    case CLOSED:
        sock = Socket();
        sstatus = CLOSE;
        break;
    case CLOSE:
        break;
    default:
        return false;
    }
    return true;
}


Session Session::closedsession() {
    Session sessoin = Session();
    sessoin.close();
    return std::move(sessoin);
}

bool Session::starttargetsession(Ipaddr target, STATUS mode) {
    if(!renew()) return false;
    if(sock.connect(target, CONNECTIMTEOUT) < 0 || sock.getsockname(this->local) < 0) {
        close();
        return false;
    }
    this->target = target;
    this->sstatus = (STATUS)mode;
    return true;
}

bool Session::startlocalsession(Ipaddr local, STATUS mode) {
    if(!renew()) return false;
    if(sock.bind(local) < 0 || sock.getsockname(this->local) < 0) {
        close();
        return false;
    }
    this->sstatus = (STATUS)mode;
    return true;
}

bool Session::startsession(Ipaddr target, Ipaddr local, STATUS mode) {
    if(!renew()) return false;
    if(sock.bind(local) < 0 || sock.connect(target, CONNECTIMTEOUT) < 0 || sock.getsockname(this->local) < 0) {
        close();
        return false;
    }
    this->sstatus = (STATUS)mode;
    this->target = target;
    return true;
}


Session Session::buildtargetsession(Ipaddr target, STATUS mode) {
    Session session;
    session.starttargetsession(target, mode);
    return std::move(session);
}

Session Session::buildlocalsession(Ipaddr local, STATUS mode) {
    Session session;
    session.startlocalsession(local, mode);
    return std::move(session);
}

Session Session::buildsession(Ipaddr target, Ipaddr local, STATUS mode) {
    Session session;
    session.startsession(target, local, mode);
    return std::move(session);
}

bool Session::listen(int backlog) {
    if(sstatus == CLOSED) return false;
    return sock.listen(backlog) >= 0;
}

Session Session::accept(STATUS mode, int sec) {
    if(sstatus == CLOSED) return closedsession();
    
    Ipaddr target;
    int flag;
    Socket nsock = sock.accept(target, flag, sec);
    if(flag < 0) {
        return closedsession();
    } 
    Session session(nsock);
    session.target = target;
    if(nsock.getsockname(session.local) < 0) {
        session.close();
        return std::move(session);
    }
    session.sstatus = (STATUS)mode;
    return std::move(session);
}

Ipaddr Session::getlocaladdr() {
    return local;
}

Ipaddr Session::gettargetaddr() {
    return target;
}

#include "session.h"
#include "logger.hpp"
#include <algorithm>
#include <exception>
#include <iostream>

int Session::wait() {
    int nbytes;
    char ch;
    while((nbytes = sock.read(&ch, 1)) > 0);
    return nbytes;
}
Session::Session() : mode(NOTCLOSE), buildstatus(-1), eoftag(true) {}

Session::Session(Session && session) {
    target = session.target;
    local = session.local;
    sock = session.sock;
    mode = session.mode;
    eoftag = session.eoftag;
    buildstatus = session.buildstatus;

    session.eoftag = true;
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
        case NOTCLOSE:
        default:
            return ;
    }
    logger("Session closed", target.port);
    sock.close();
}

void Session::sendmsg(const std::string& msg) {
    logger("SEND: " + msg, target.port);
    std::string tmsg = msg;
    if(tmsg.back() != '\n') tmsg.push_back('\n'); // 换行符为结尾
    if(sock.write(tmsg.c_str(), tmsg.size()) < 0) {
        throw "session closed";
    }
}


void Session::sendstream(std::istream& is) {
    char buffer[BUFFER_SIZE];
    int nbytes;
    do {
        is.read(buffer, sizeof(buffer));
        nbytes = is.gcount();
        if(sock.write(buffer, nbytes) < 0) {
            throw "session closed";
        }
    } while(nbytes);
    
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
        throw "session closed";
    }
}

void Session::recvstream(std::ostream& os) {
    char buffer[BUFFER_SIZE];
    int nbytes;
    while((nbytes = sock.read(buffer, sizeof(buffer))) > 0) {
        os.write(buffer, nbytes);
    }
    if(nbytes < 0) {
        throw "session closed";
    }
}

void Session::gettok(std::string& cmd) {
    cmd.clear();
    if(eoftag) return ;
    int nbytes;
    char ch;
    while((nbytes = sock.read(&ch, 1)) > 0) {
        if(ch == '\n') {
            eoftag = true;
            break;
        }
        if(!isspace(ch)) break;
    }
    if(nbytes <= 0)  {
        throw "session closed";
    }
    if(eoftag) return ;
    cmd.push_back(ch);
    while((nbytes = sock.read(&ch, 1)) > 0) {
        if(ch == '\n') {
            eoftag = true;
            break;
        }
        if(isspace(ch)) break;
        cmd.push_back(ch);
    }
    if(nbytes <= 0)  {
        throw "session closed";
    }
    logger("RECV TOKEN: " + cmd, target.port);
}

void Session::nextline() {
    if(eoftag) {
        eoftag = false;
        return ;
    }
    int nbytes;
    char ch;
    while((nbytes = sock.read(&ch, 1)) > 0) {
        if(ch == '\n') {
            break;
        }
    }
}

// no sure if it can work
bool Session::status() {
    if(buildstatus < 0) return false;
    return true;
}
Session Session::buildtargetsession(Ipaddr target, CLOSEMODE mode) {
    Session session;
    Socket sock;
    int flag = sock.connect(target);
    session.buildstatus = flag;
    if(flag < 0) {
        return std::move(session);
    }
    session.mode = mode;
    session.target = target;
    sock.getsockname(session.local);
    session.sock = sock;
    return std::move(session);
}

Session Session::buildlocalsession(Ipaddr local, CLOSEMODE mode) {
    Session session;
    Socket sock;
    int flag = sock.bind(local);
    session.buildstatus = flag;
    if(flag < 0) {
        return std::move(session);
    }
    session.mode = mode;
    sock.getsockname(session.local);
    session.sock = sock;
    return std::move(session);

}


Session Session::buildsession(Ipaddr target, Ipaddr local, CLOSEMODE mode) {
    Session session;
    Socket sock;
    int flag = sock.bind(local);
    if(flag < 0) {
        return std::move(session);
    }
    flag = sock.connect(target);
    session.buildstatus = flag;
    if(flag < 0) {
        return std::move(session);
    }
    session.mode = mode;
    session.target = target;
    sock.getsockname(session.local);
    session.sock = sock;
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

Session Session::accept(int sec) {
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
    session.sock = nsock;
    session.target = target;
    nsock.getsockname(session.local);
    session.mode = PASSIVE;
    return std::move(session);
}

int Session::getlocalport() {
    if(status()) {
        return local.port;
    }
    return -1;
}

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cassert>
#include <iostream>
#include "socket.h"
#include "logger.hpp"

static int protocolCv(Protocol protocol) {
    switch(protocol) {
        case P_TCP: return SOCK_STREAM;
        case P_UDP: return SOCK_DGRAM;
        default: return SOCK_STREAM;
    }
}

static int ipTypeCv(Iptype ipType) {
    switch(ipType) {
        case IPV4: return AF_INET;
        default: return AF_INET;
    }
}

static Iptype ipTypeCv(int af) {
    switch(af) {
        case AF_INET: return IPV4;
        default: return IPV4;
    }
}

static int shutdownCv(ShutdownType howto) {
    switch(howto) {
        case SD_RD  : return SHUT_RD; 
        case SD_WR  : return SHUT_WR;
        case SD_RDWR: return SHUT_RDWR;
        default: panic("sd error");
    }
    return -1;
}

static void addrCv(const Ipaddr& ipaddr, sockaddr* addr) {
    memset(addr, 0, sizeof(sockaddr));
    sockaddr_in *serv_addr = (sockaddr_in *)addr;
    switch(ipaddr.ipType) {
        case IPV4:
            serv_addr->sin_family = ipTypeCv(ipaddr.ipType);                //使用IPv4地址
            serv_addr->sin_addr.s_addr = inet_addr(ipaddr.addr.c_str());    //具体的IP地址
            serv_addr->sin_port = htons(ipaddr.port);                       //端口
            break;
        default:
            panic("addrcv ip to addr error");
    }
}

static void addrCv(const sockaddr* addr, Ipaddr& ipaddr) {
    switch(addr->sa_family) {
        case AF_INET:
            ipaddr.addr = inet_ntoa( ((sockaddr_in*)addr)->sin_addr);
            ipaddr.ipType = ipTypeCv(((sockaddr_in*)addr)->sin_family);
            ipaddr.port = ntohs(((sockaddr_in*)addr)->sin_port);
            break;
        default:
            panic("addrcv addr to ip error");
    }
}

Ipaddr::Ipaddr() : addr("0.0.0.0"), port(0), ipType(IPV4) {}

Ipaddr::Ipaddr(const std::string &addr, int port, Iptype ipType) : addr(addr), port(port), ipType(ipType) {}

Socket::Socket(Protocol protocol, Iptype ipType) : protocol(protocol), ipType(ipType) {
    int af = ipTypeCv(ipType), type = protocolCv(protocol);
    sock = socket(af, type, 0);
}



int Socket::bind(const Ipaddr& addr) {
    sockaddr serv_addr;
    addrCv(addr, &serv_addr);

    return ::bind(sock, &serv_addr, sizeof(sockaddr_in));
}

int Socket::connect(const Ipaddr& addr) {
    sockaddr serv_addr;
    addrCv(addr, &serv_addr);
    return ::connect(sock, &serv_addr, sizeof(sockaddr));
}


int Socket::getsockname(Ipaddr& ipaddr) {
    struct sockaddr addr;
    socklen_t socklen;
    // 如果绑定时设置了端口号为0，用这个获得绑定的地址从而获得系统随机分配的端口号
    int flag = ::getsockname(sock, &addr, &socklen);
    logger(sock, "sock");
    logger(flag, "flag");
    logger(ntohs(((sockaddr_in*)&addr)->sin_port), "port");
    if(flag < 0) return flag;
    addrCv(&addr, ipaddr);
    return flag;
}

int Socket::setsendtimeout(int sec) {
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;
    socklen_t len = sizeof(timeout);
	return setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
}

int Socket::setrecvtimeout(int sec) {
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;
    socklen_t len = sizeof(timeout);
	return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, len);
}

int Socket::listen(int backlog) {
    return ::listen(sock, backlog);
}


int Socket::accept(Ipaddr& addr, Socket& sock) {
    sockaddr serv_addr;
    socklen_t addr_size = sizeof(sockaddr);
    int fd = ::accept(this->sock, &serv_addr, &addr_size);
    if(fd < 0) return fd;
    sock.sock = fd;
    addrCv(&serv_addr, addr);
    return fd;
}

int Socket::shutdown(ShutdownType howto) {
    return ::shutdown(sock, shutdownCv(howto));
}

int Socket::read(void* buf, size_t nbytes) {
    return ::read(sock, buf, nbytes);
}

int Socket::write(const void* buf, size_t nbytes) {
    return ::write(sock, buf, nbytes);
}

int Socket::close() {
    return ::close(sock);
}

int Socket::recvfrom(void* buf, size_t nbytes, Ipaddr& from) {
    sockaddr fromAddr;
    socklen_t addrLen = sizeof(sockaddr);
    int flag = ::recvfrom(sock, buf, nbytes, 0, &fromAddr, &addrLen);
    if(flag < 0) return flag;
    addrCv(&fromAddr, from);
    return flag;
}

int Socket::sendto(void *buf, size_t nbytes, const Ipaddr& to) {
    sockaddr toAddr;
    addrCv(to, &toAddr);
    socklen_t addrLen = sizeof(sockaddr);
    return ::sendto(sock, buf, nbytes, 0, &toAddr, addrLen);
}




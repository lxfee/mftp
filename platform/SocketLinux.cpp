#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cassert>
#include <iostream>
#include "socket.h"

struct SOCK {
    SOCK(int sock) : sock(sock) {}
    int sock;
};

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
    sockaddr_in *serv_addr = (sockaddr_in *)addr;
    switch(ipaddr.ipType) {
        case IPV4:
            memset(serv_addr, 0, sizeof(sockaddr));  //每个字节都用0填充
            serv_addr->sin_family = ipTypeCv(ipaddr.ipType);  //使用IPv4地址
            serv_addr->sin_addr.s_addr = inet_addr(ipaddr.addr.c_str());  //具体的IP地址
            serv_addr->sin_port = htons(ipaddr.port);  //端口
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

Socket::Socket(Protocol protocol, Iptype ipType) : protocol(protocol), ipType(ipType) {
    int af = ipTypeCv(ipType), type = protocolCv(protocol);
    sock = std::make_shared<SOCK>(socket(af, type, 0));
}

Ipaddr::Ipaddr(const std::string &addr, int port, Iptype ipType) : addr(addr), port(port), ipType(ipType) {}
Ipaddr::Ipaddr() = default;

int Socket::bind(const Ipaddr& addr) {
    sockaddr serv_addr;
    addrCv(addr, &serv_addr);
    return ::bind(sock->sock, &serv_addr, sizeof(sockaddr));
}

int Socket::connect(const Ipaddr& addr) {
    sockaddr serv_addr;
    addrCv(addr, &serv_addr);
    return ::connect(sock->sock, &serv_addr, sizeof(sockaddr));
}


int Socket::listen(int backlog) {
    return ::listen(sock->sock, backlog);
}


Socket Socket::accept(Ipaddr& addr) {
    sockaddr serv_addr;
    socklen_t addr_size = sizeof(sockaddr);
    Socket res(*this);
    res.sock = std::make_shared<SOCK>(::accept(sock->sock, &serv_addr, &addr_size));
    addrCv(&serv_addr, addr);
    return res;
}

int Socket::shutdown(ShutdownType howto) {
    return ::shutdown(sock->sock, shutdownCv(howto));
}

int Socket::read(void* buf, size_t nbytes) {
    return ::read(sock->sock, buf, nbytes);
}

int Socket::write(const void* buf, size_t nbytes) {
    return ::write(sock->sock, buf, nbytes);
}


int Socket::close() {
    return ::close(sock->sock);
}

int Socket::recvfrom(void* buf, size_t nbytes, Ipaddr& from) {
    sockaddr fromAddr;
    socklen_t addrLen = sizeof(sockaddr);
    int res = ::recvfrom(sock->sock, buf, nbytes, 0, &fromAddr, &addrLen);
    addrCv(&fromAddr, from);
    return res;
}

int Socket::sendto(void *buf, size_t nbytes, const Ipaddr& to) {
    sockaddr toAddr;
    addrCv(to, &toAddr);
    socklen_t addrLen = sizeof(sockaddr);
    int res = ::sendto(sock->sock, buf, nbytes, 0, &toAddr, addrLen);
    return res;
}




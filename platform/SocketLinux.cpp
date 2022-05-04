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




static int shutdownconvert(SDType howto) {
    switch(howto) {
        case SD_RD  : return SHUT_RD; 
        case SD_WR  : return SHUT_WR;
        case SD_RDWR: return SHUT_RDWR;
        default: panic("sd error");
    }
    return -1;
}

static void addrconvert(const Ipaddr& from, struct sockaddr_in& to) {
    memset(&to, 0, sizeof(struct sockaddr_in));
    to.sin_family = AF_INET;
    to.sin_port = htons(from.port);
    to.sin_addr.s_addr = (uint32_t)from.addr;
}



static void addrconvert(const struct sockaddr_in& from, Ipaddr& to) {
    to.addr = from.sin_addr.s_addr;
    to.port = ntohs(from.sin_port);
}

Ipaddr::Ipaddr() : addr(0), port(0) {}

Ipaddr::Ipaddr(const std::string &addr, int port) : port(port) {
    this->addr = inet_addr(addr.c_str());
}

std::string Ipaddr::getaddr() {
    in_addr taddr;
    taddr.s_addr = addr;
    std::string res = inet_ntoa(taddr);
    return res;
}
void Ipaddr::setaddr(std::string addr) {
    this->addr = inet_addr(addr.c_str());
}
std::string Ipaddr::to_string() {
    return getaddr() + ":" + std::to_string(port);
}

Socket::Socket() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
}



int Socket::bind(Ipaddr addr) {
    struct sockaddr_in serv_addr;
    addrconvert(addr, serv_addr);
    return ::bind(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
}

int Socket::connect(Ipaddr addr) {
    struct sockaddr_in serv_addr;
    addrconvert(addr, serv_addr);
    return ::connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
}


int Socket::getsockname(Ipaddr& addr) {
    struct sockaddr_in tmpaddr;
    // 不要忘记设置长度
    socklen_t socklen = sizeof(tmpaddr);
    // 如果绑定时设置了端口号为0，用这个获得绑定的地址从而获得系统随机分配的端口号
    int flag = ::getsockname(sock, (struct sockaddr *)&tmpaddr, &socklen);
    if(flag < 0) return flag;
    addrconvert(tmpaddr, addr);
    return flag;
}


int Socket::checkreadable(int sec) {
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;
    // fd_set: 套接字集合
    fd_set socks;
    socks.fds_bits[0] = sock;
    // select函数可以观察套接字集合中是否可读可写或出错（从第2~4参数传入套接字集合）
    // 如果不可读/写/无出错就阻塞
    // 阻塞时间由第5个参数决定，NULL代表一直阻塞
    // 第一个参数填0接口，听说是为了和linux兼容
    // 返回值为准备好的套接字的个数
    return ::select(1, &socks, NULL, NULL, &timeout);
}

int Socket::checkwriteable(int sec) {
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;
    fd_set socks;
    socks.fds_bits[0] = sock;
    return ::select(1, NULL, &socks, NULL, &timeout);
}
int Socket::checkerro(int sec) {
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;
    fd_set socks;
    socks.fds_bits[0] = sock;
    return ::select(1, NULL, NULL, &socks, &timeout);
}

int Socket::listen(int backlog) {
    return ::listen(sock, backlog);
}


Socket Socket::accept(Ipaddr& addr, int& status) {
    struct sockaddr_in tmpaddr;
    socklen_t addr_size = sizeof(tmpaddr);
    status = ::accept(this->sock, (struct sockaddr*)&tmpaddr, &addr_size);
    Socket sock(*this);
    sock.sock = status;
    if(status < 0) return sock;
    addrconvert(tmpaddr, addr);
    return sock;
}

int Socket::shutdown(SDType howto) {
    return ::shutdown(sock, shutdownconvert(howto));
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
    struct sockaddr_in fromAddr;
    socklen_t addrLen = sizeof(fromAddr);
    int flag = ::recvfrom(sock, buf, nbytes, 0, (struct sockaddr*)&fromAddr, &addrLen);
    if(flag < 0) return flag;
    addrconvert(fromAddr, from);
    return flag;
}

int Socket::sendto(void *buf, size_t nbytes, Ipaddr to) {
    struct sockaddr_in toAddr;
    addrconvert(to, toAddr);
    socklen_t addrLen = sizeof(toAddr);
    return ::sendto(sock, buf, nbytes, 0, (struct sockaddr*)&toAddr, addrLen);
}




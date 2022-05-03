#include <winsock2.h>
#include "socket.h"
#include <iostream>
#include <cassert>

void WSAStart() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void WSAClean() {
    WSACleanup();
}

static int shutdown(SDType howto) {
    switch(howto) {
        case SD_RD  : return SD_RECEIVE; 
        case SD_WR  : return SD_SEND;
        case SD_RDWR: return SD_BOTH;
        default: panic("sd error");
    }
    return -1;
}

static void addrconvert(const Ipaddr& from, struct sockaddr_in& to) {
    // memset(&to, 0, sizeof(struct sockaddr_in));
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

Socket::Socket() {
    static bool first = true;
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
    int socklen = sizeof(tmpaddr);
    // 如果绑定时设置了端口号为0，用这个获得绑定的地址从而获得系统随机分配的端口号
    int flag = ::getsockname(sock, (struct sockaddr *)&tmpaddr, &socklen);
    if(flag < 0) return flag;
    addrconvert(tmpaddr, addr);
    return flag;
}

int Socket::setsendtimeout(int sec) {
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;
    int len = sizeof(timeout);
	return setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, len);
}

int Socket::setrecvtimeout(int sec) {
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;
    int len = sizeof(timeout);
	return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, len);
}

int Socket::listen(int backlog) {
    return ::listen(sock, backlog);
}


Socket Socket::accept(Ipaddr& addr, int& status) {
    struct sockaddr_in tmpaddr;
    int addr_size = sizeof(tmpaddr);
    status = ::accept(this->sock, (struct sockaddr*)&tmpaddr, &addr_size);
    Socket sock(*this);
    sock.sock = status;
    if(status < 0) return sock;
    addrconvert(tmpaddr, addr);
    return sock;
}

int Socket::shutdown(SDType howto) {
    return ::shutdown(sock, SDType(howto));
}

int Socket::read(void* buf, size_t nbytes) {
    return ::recv(sock, (char*)buf, nbytes, 0);
}

int Socket::write(const void* buf, size_t nbytes) {
    return ::send(sock, (char*)buf, nbytes, 0);
}

int Socket::close() {
    return ::closesocket(sock);
}

int Socket::recvfrom(void* buf, size_t nbytes, Ipaddr& from) {
    struct sockaddr_in fromAddr;
    int addrLen = sizeof(fromAddr);
    int flag = ::recvfrom(sock, (char*)buf, nbytes, 0, (struct sockaddr*)&fromAddr, &addrLen);
    if(flag < 0) return flag;
    addrconvert(fromAddr, from);
    return flag;
}

int Socket::sendto(void *buf, size_t nbytes, Ipaddr to) {
    struct sockaddr_in toAddr;
    addrconvert(to, toAddr);
    int addrLen = sizeof(toAddr);
    return ::sendto(sock, (char*)buf, nbytes, 0, (struct sockaddr*)&toAddr, addrLen);
}




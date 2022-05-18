#include <winsock2.h>
#include "socket.h"
#include <iostream>
#include <cassert>
#include <logger.hpp>



static int shutdownconvert(SDType howto) {
    switch(howto) {
        case SD_RD  : return SD_RECEIVE; 
        case SD_WR  : return SD_SEND;
        case SD_RDWR: return SD_BOTH;
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

std::string Ipaddr::to_string() {
    return getaddr() + ":" + std::to_string(port);
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

// sec代表connect超时时间
int Socket::connect(Ipaddr addr, int sec) {
    struct sockaddr_in serv_addr;
    addrconvert(addr, serv_addr);
    unsigned long iMode = 1;
    // 设置为非阻塞模式
	int iResult = ioctlsocket(sock, FIONBIO, &iMode);
	if (iResult != NO_ERROR) return -1;
	if(::connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == false) return -1;

    fd_set Write, Err;
	FD_ZERO(&Write);
	FD_ZERO(&Err);
	FD_SET(sock, &Write);
	FD_SET(sock, &Err);
    
    TIMEVAL Timeout;
	Timeout.tv_sec = sec;
    Timeout.tv_usec = 0;
	// 检查socket在sec秒内是否可读
	select(0, NULL, &Write, &Err, &Timeout);
	iMode = 0;
	// 设回阻塞模式
    iResult = ioctlsocket(sock, FIONBIO, &iMode);
	if (iResult != NO_ERROR) return -1;
    if (FD_ISSET(sock, &Write)) {
		return 0;
	}
    
    return -1;
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

int Socket::checkreadable(int sec) {
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;
    // fd_set: 套接字集合
    fd_set socks;
    FD_ZERO(&socks);
    FD_SET(sock, &socks);
    // select函数可以观察套接字集合中是否可读可写或出错（从第2~4参数传入套接字集合）
    // 如果不可读/写/无出错就阻塞
    // 阻塞时间由第5个参数决定，NULL代表一直阻塞
    // 第一个参数填0接口，听说是为了和linux兼容
    // 返回值为准备好的套接字的情况
    return ::select(0, &socks, NULL, NULL, &timeout);
}

int Socket::checkwriteable(int sec) {
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;
    fd_set socks;
    FD_ZERO(&socks);
    FD_SET(sock, &socks);
    return ::select(0, NULL, &socks, NULL, &timeout);
}
int Socket::checkerro(int sec) {
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;
    fd_set socks;
    FD_ZERO(&socks);
    FD_SET(sock, &socks);
    return ::select(0, NULL, NULL, &socks, &timeout);
}

int Socket::listen(int backlog) {
    return ::listen(sock, backlog);
}


Socket Socket::accept(Ipaddr& addr, int& status, int sec) {
    Socket nsock(*this);
    if(sec >= 0) {
        if(!checkreadable(sec)) {
            status = -1;
            nsock.sock = -1;
            return nsock;
        }
    }
    struct sockaddr_in tmpaddr;
    int addr_size = sizeof(tmpaddr);
    nsock.sock = ::accept(this->sock, (struct sockaddr*)&tmpaddr, &addr_size);
    status = nsock.sock;
    if(status < 0) return nsock;
    addrconvert(tmpaddr, addr);
    return nsock;
}

int Socket::shutdown(SDType howto) {
    return ::shutdown(sock, shutdownconvert(howto));
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




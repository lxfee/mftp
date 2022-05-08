// only suport ipv4 tcp
#pragma once
#include <string>
#include <memory>
#include <iostream>

#ifdef WINDOWS
#include <winsock2.h>
using SOCK_T = SOCKET;
#endif
#ifdef LINUX
using SOCK_T = int;
#endif

using ADDR_T = unsigned int;

#define panic(msg) std::cerr << msg << std::endl; assert(0)

enum SDType {SD_RD = 1, SD_WR = 2, SD_RDWR = 3};

struct Ipaddr {
	Ipaddr();
	Ipaddr(const std::string &addr, int port);
	std::string getaddr();
	void setaddr(std::string addr);
	int port;
	// warning: do not set addr manually
	ADDR_T addr;
	std::string to_string();
};

// 封装
class Socket {
public:
	Socket();
	int bind(Ipaddr addr);
	int connect(Ipaddr addr, int sec);
	int listen(int backlog);
	Socket accept(Ipaddr& addr, int& status, int sec);
	int shutdown(SDType howto);
	int read(void* buf, size_t nbytes);
	int write(const void* buf, size_t nbytes);
	int close();
	int recvfrom(void* buf, size_t nbytes, Ipaddr& from);
	int sendto(void *buf, size_t nbytes, Ipaddr to);
	int getsockname(Ipaddr& ipaddr);
	int checkreadable(int sec);
	int checkwriteable(int sec);
	int checkerro(int sec);

private:
	SOCK_T sock;		// sock套接字
};
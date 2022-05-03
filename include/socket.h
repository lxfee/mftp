// only suport ipv4 tcp
#pragma once
#include <string>
#include <memory>

using SOCK_T = int;
using ADDR_T = unsigned int;

#define panic(msg) std::cerr << msg << std::endl; assert(0)

enum ShutdownType {SD_RD = 1, SD_WR = 2, SD_RDWR = 3};

struct Ipaddr {
	Ipaddr();
	Ipaddr(const std::string &addr, int port);
	std::string getaddr();
	void setaddr(std::string addr);
	int port;
	// warning: do not set addr manually
	ADDR_T addr;
};

class Socket {
public:
	Socket();
	int bind(Ipaddr addr);
	int connect(Ipaddr addr);
	int listen(int backlog);
	Socket accept(Ipaddr& addr, int& status);
	int shutdown(ShutdownType howto);
	int read(void* buf, size_t nbytes);
	int write(const void* buf, size_t nbytes);
	int close();
	int recvfrom(void* buf, size_t nbytes, Ipaddr& from);
	int sendto(void *buf, size_t nbytes, Ipaddr to);
	int getsockname(Ipaddr& ipaddr);
	int setsendtimeout(int sec);
	int setrecvtimeout(int sec);
	
private:
	SOCK_T sock;		// sock套接字
};
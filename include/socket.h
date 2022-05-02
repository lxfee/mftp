#pragma once
#include <string>
#include <memory>
#define _LINUX 0
#ifdef _LINUX
	using SOCK_T = int;
#endif


#define panic(msg) std::cerr << msg << std::endl; assert(0)

enum Iptype {IPV4};
enum Protocol {P_TCP, P_UDP};
enum ShutdownType {SD_RD, SD_WR, SD_RDWR};

struct Ipaddr {
	Ipaddr();
	Ipaddr(const std::string &addr, int port, Iptype ipType = IPV4);
	std::string addr;
	int port;
	Iptype ipType; // IPV4
};

class Socket {
public:
	Socket(Protocol protocol = P_TCP, Iptype ipType = IPV4);
	int bind(const Ipaddr& addr);
	int connect(const Ipaddr& addr);
	int listen(int backlog);
	int accept(Ipaddr& addr, Socket& sock);
	int shutdown(ShutdownType howto);
	int read(void* buf, size_t nbytes);
	int write(const void* buf, size_t nbytes);
	int close();
	int recvfrom(void* buf, size_t nbytes, Ipaddr& from);
	int sendto(void *buf, size_t nbytes, const Ipaddr& to);
	int getsockname(Ipaddr& ipaddr);
	int setsendtimeout(int sec);
	int setrecvtimeout(int sec);
	
private:
	SOCK_T sock;
	Iptype ipType; // IPV4
	Protocol protocol; // P_TCP, P_UDP
};
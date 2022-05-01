#pragma once
#include <string>
#include <memory>
#define panic(msg) std::cerr << msg << std::endl; assert(0)

enum Iptype {IPV4};
enum Protocol {P_TCP, P_UDP};
enum ShutdownType {SD_RD, SD_WR, SD_RDWR};
struct SOCK;
typedef std::shared_ptr<SOCK> SOCK_T;

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
	Socket accept(Ipaddr& addr);
	int shutdown(ShutdownType howto);
	int read(void* buf, size_t nbytes);
	int write(const void* buf, size_t nbytes);
	int close();
	int recvfrom(void* buf, size_t nbytes, Ipaddr& from);
	int sendto(void *buf, size_t nbytes, const Ipaddr& to);
	
private:
	Iptype ipType; // IPV4
	Protocol protocol; // P_TCP, P_UDP
	SOCK_T sock;
};
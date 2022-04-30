#pragma once
#include "socket.h"
#include <string>
#include <queue>
#include <iostream>

#define BUFFER_SIZE 512


int sendmsg(Socket& sock, const std::string& msg);
int sendstream(Socket& sock, std::istream& os);
int recvmsg(Socket& sock, std::string& smg);
int recvstream(Socket& sock, std::ostream& is);


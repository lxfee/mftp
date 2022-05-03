idir = include
objs = SocketLinux.o session.o utils.o


all : client server

server: servermain.o $(objs) server.o
	g++ -g $(objs) servermain.o server.o -o server -lpthread

client : clientmain.o $(objs) client.o
	g++ -g clientmain.o $(objs) client.o -o client


servermain.o : servermain.cpp
	g++ -I$(idir)  -c -g servermain.cpp

clientmain.o : clientmain.cpp
	g++ -I$(idir) -c -g clientmain.cpp

server.o : server.cpp
	g++ -std=c++17 -I$(idir) -c -g server.cpp

SocketLinux.o : platform/SocketLinux.cpp
	g++ -I$(idir) -c -g platform/SocketLinux.cpp

session.o : session.cpp
	g++ -I$(idir) -c -g session.cpp

utils.o : utils.cpp
	g++ -std=c++17 -I$(idir) -c -g utils.cpp

client.o : client.cpp
	g++  -std=c++17 -I$(idir) -c -g client.cpp

clean :
	-rm -f *.o server client
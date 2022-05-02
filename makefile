idir = include
objs = SocketLinux.o session.o 


all : client server

server: servermain.o $(objs) server.o
	g++ -g $(objs) servermain.o server.o -o server -lpthread

client : client.o $(objs)
	g++ -g client.o $(objs) -o client


servermain.o : servermain.cpp
	g++ -I$(idir)  -c -g servermain.cpp

client.o : client.cpp
	g++ -I$(idir) -c -g client.cpp

server.o : server.cpp
	g++ -I$(idir) -c -g server.cpp

SocketLinux.o : platform/SocketLinux.cpp
	g++ -I$(idir) -c -g platform/SocketLinux.cpp

session.o : session.cpp
	g++ -I$(idir) -c -g session.cpp

clean :
	-rm -f *.o server client
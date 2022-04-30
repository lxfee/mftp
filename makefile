idir = include
objs = SocketLinux.o session.o

all : client server

client : client.o $(objs)
	g++ -g client.o $(objs) -o client

server : server.o $(objs)
	g++ -g server.o $(objs) -o server

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
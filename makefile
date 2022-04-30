all : client server
idir = include


client : client.o SocketLinux.o
	g++ -g client.o SocketLinux.o -o client

server : server.o SocketLinux.o
	g++ -g server.o SocketLinux.o -o server

client.o : client.cpp
	g++ -I$(idir) -c -g client.cpp

server.o : server.cpp
	g++ -I$(idir) -c -g server.cpp

SocketLinux.o : platform/SocketLinux.cpp
	g++ -I$(idir) -c -g platform/SocketLinux.cpp

clean :
	-rm -f *.o server client
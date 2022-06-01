platform = 
flag = 
lib =  
include_dir = include
objs = session.o utils.o
serverobjs = servermain.o server.o
clientobjs = clientmain.o client.o

all : client server

ifeq ($(platform), windows)
serverobjs += SocketWindows.o
clientobjs += SocketWindows.o
flag += -DWINDOWS
lib += -lws2_32
else
serverobjs += SocketLinux.o
clientobjs += SocketLinux.o
flag += -DLINUX 
endif

server:  $(serverobjs) $(objs)
	g++ -g $(flag) $(serverobjs) $(objs) -o server  -lpthread $(lib)


client :  $(clientobjs) $(objs)
	g++ -g $(flag) $(clientobjs) $(objs) -o client  $(lib)


servermain.o : servermain.cpp
	g++ $(flag) -I$(include_dir)  -c -g servermain.cpp

clientmain.o : clientmain.cpp
	g++ $(flag) -I$(include_dir) -c -g clientmain.cpp


SocketLinux.o : platform/SocketLinux.cpp
	g++ -I$(include_dir) -DLINUX -c -g platform/SocketLinux.cpp

SocketWindows.o : platform/SocketWindows.cpp
	g++ -I$(include_dir) -DWINDOWS -c -g platform/SocketWindows.cpp

session.o : session.cpp
	g++ $(flag) -I$(include_dir) -c -g session.cpp


utils.o : utils.cpp
	g++ $(flag) -std=c++17 -I$(include_dir) -c -g utils.cpp

client.o : client.cpp
	g++ $(flag) -std=c++17 -I$(include_dir) -c -g client.cpp

server.o : server.cpp
	g++ $(flag) -std=c++17 -I$(include_dir) -c -g server.cpp


clean :
ifeq ($(platform), windows)
	-del *.o *.exe
else
	-rm -f *.o server client
endif
	
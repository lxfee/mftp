.PHONY : clean all

# 构建需要的变量
CC = g++
serverobjects = servermain.o server.o
clientobjects = clientmain.o client.o
objects = session.o utils.o
IncludeDir = include
CFLAG = -MMD -std=c++17
platform = 
LIB = pthread
VPATH = $(includedir):platform

ifeq ($(platform), windows)
objects += SocketWindows.o
CFLAG += -DWINDOWS
LIB += ws2_32
else
objects += SocketLinux.o
CFLAG += -DLINUX 
endif

# 构建目标
all : client server


client: $(objects) $(clientobjects)
	$(CC) $(objects) $(clientobjects) $(LIB:%=-l%) -o client

server: $(objects) $(serverobjects)
	$(CC) $(objects) $(serverobjects) $(LIB:%=-l%) -o server

$(objects) $(serverobjects) $(clientobjects): %.o: %.cpp
	$(CC) -I$(IncludeDir) $(CFLAG) -c $< -o $@

-include *.d


# 其它指令
clean :
ifeq ($(platform), windows)
	-del *.o *.d server.exe client.exe
else
	-rm -f *.o *.d server client
endif
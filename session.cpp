#include "session.h"


int sendmsg(Socket& sock, const std::string& msg) {
    std::string tmsg = msg;
    if(tmsg.back() != '\n') tmsg.push_back('\n'); // 换行符为结尾
    return sock.write(tmsg.c_str(), tmsg.size());
}

int sendstream(Socket& sock, std::istream& is) {
    char buffer[BUFFER_SIZE];
    int nbytes;
    int totbytes = 0;
    do {
        is.read(buffer, sizeof(buffer));
        nbytes = is.gcount();
        if(sock.write(buffer, nbytes) < 0) break; 
        totbytes += nbytes;
    } while(nbytes);
    return totbytes;
}

int recvmsg(Socket& sock, std::string& msg) {
    msg.clear();
    char ch;
    int nbytes;
    int totbytes = 0;
    while((nbytes = sock.read(&ch, 1)) > 0) {
        totbytes++;
        if(ch == '\n') break;
        msg.push_back(ch);
    }
    if(nbytes < 0) return -1;
    return totbytes;
}

int recvstream(Socket& sock, std::ostream& os) {
    char buffer[BUFFER_SIZE];
    int nbytes;
    int totbytes = 0;
    while((nbytes = sock.read(buffer, sizeof(buffer))) > 0) {
        totbytes += nbytes;
        os.write(buffer, nbytes);
    }
    return totbytes;
}
#include "socket.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include "session.h"

using namespace std;

int main() {
    Socket serversock;
    cout << serversock.bind(Ipaddr("127.0.0.1", 1234)) << endl;
    serversock.listen(20);
    Ipaddr clientIp;
    Socket clientsock = serversock.accept(clientIp);
    cout << clientIp.addr << endl;
    cout << clientIp.port << endl;
    
    ofstream fout;
    fout.open("1.txt", ios::binary);
    recvstream(clientsock, fout);

    clientsock.close();
    serversock.close();
    return 0;
}

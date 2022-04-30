#include <iostream>
#include "socket.h"
#include "session.h"
#include <fstream>

using namespace std;

int main() {
    Socket clientsock;
    cout << clientsock.connect(Ipaddr("127.0.0.1", 1234)) << endl;
    char buf[100];
    
    ifstream fin;
    fin.open("2.txt", ios::binary);
    sendstream(clientsock, fin);


    clientsock.shutdown(SD_WR);
    clientsock.read(0, 0);
    clientsock.close();
}
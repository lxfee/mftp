#include <iostream>
#include "Socket.h"

using namespace std;

int main() {
    Socket client;
    cout << client.connect(Ipaddr("127.0.0.1", 1234)) << endl;
    char buf[100];
    while(client.read(buf, sizeof buf) > 0) {
        cout << buf << endl;
    }
    client.close();
}
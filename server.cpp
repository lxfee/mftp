#include "Socket.h"
#include <iostream>
#include <cstring>

using namespace std;

int main() {
    Socket server;
    cout << server.bind(Ipaddr("127.0.0.1", 1234)) << endl;
    server.listen(20);

    while(1) {
        Ipaddr clientIp;
        Socket session = server.accept(clientIp);
        cout << clientIp.addr << endl;
        cout << clientIp.port << endl;
        char *buf = "hello client!";
        session.write(buf, strlen(buf) + 1);
        session.close();
    }
    server.close();
    return 0;
}

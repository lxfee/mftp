#include <iostream>
#include "socket.h"
#include "session.h"
#include <fstream>
#include <sstream>
#include "logger.hpp"
#include "utils.h"


using namespace std;
#define CMDPORT  1234

void prasecmd(string cmd) {
    
}

int main() {
    string cmd;
    
    while(1) {
        cout << "mftp> ";
        getline(cin, cmd);
        if(cmd.empty()) continue;
        prasecmd(cmd);
    }
}
#include "client.h"
#include <algorithm>
#include <iomanip>
#include <vector>
#include <filesystem>
#include <fstream>
#include "socket.h"
#include "utils.h"

CONNECTMODE mode = PASV;
namespace fs = std::filesystem;
using namespace std;
std::string buffer;

void gettok(std::string& cmd) {
    cmd.clear();
    while(!buffer.empty()) {
        if(isspace(buffer.back())) buffer.pop_back();
        else break;
    }
    while(!buffer.empty()) {
        if(!isspace(buffer.back())) {
            cmd.push_back(buffer.back());
            buffer.pop_back();
        }
        else break;
    }
}

void readline(std::string& line) {
    line = buffer;
    while(isspace(line.back())) line.pop_back();
    std::reverse(line.begin(), line.end());
}

void nextline() {
    getline(std::cin, buffer);
    std::reverse(buffer.begin(), buffer.end());
}

bool excpet(std::string exp) {
    nextline();
    std::string rel;
    gettok(rel);
    return exp == rel;
}

vector<string> cmds = {
     "ls"           , "列出目录下的文件和文件夹"
    ,"help"         , "帮助"
    ,"status"       , "查看连接状态"
    ,"open"         , "打开一个连接"
    ,"close"        , "关闭当前连接"
    ,"bye"          , "关闭客户端"
    ,"time"         , "查询服务器时间"
    ,"get"          , "获得文件"
    ,"put"          , "发送文件"
    ,"run"          , "运行远程指令"
};

void printhelp() {
    string cmd;
    gettok(cmd);
    if(cmd.empty()) {
        cout << "输入help [cmd]查看帮助" << endl;
        for(int i = 0; i < cmds.size(); i += 2) {
            cout << setw(10) << cmds[i];
            if((i / 2 + 1) % 4 == 0) cout << endl;
        }
        cout << endl;
    } else {
        bool found = false;
        for(int i = 0; i < cmds.size(); i+= 2) {
            if(cmds[i] == cmd) {
                cout << cmd << ": " << cmds[i + 1] << endl;
                found = true;
                break;
            }
        }
        if(!found) cout << cmd << ": " << "找不到指令" << endl;
    }
}

Session buildstream(Session& scmd, CONNECTMODE mode) {
    string cmd;
    if(mode == PASV) {
        scmd.sendmsg("PASV");
        if(!scmd.expect("PORT")) return Session::closedsession();
        scmd.gettok(cmd);
        Ipaddr target = scmd.gettargetaddr();
        target.port = atoi(cmd.c_str()); 
    
        Ipaddr local = scmd.getlocaladdr();
        local.port = 0;
        Session session = Session::buildsession(target, local, ACTIVE);
        if(!session.status()) {
            logger("ERR: can not build data connection", "buildstream");
            return std::move(session);
        }
        return std::move(session);
    } else /*ACTV*/ {
        Ipaddr local = scmd.getlocaladdr();
        local.port = 0;
        Session session = Session::buildlocalsession(local, CLOSE);
        if(!session.status()) {
            logger("ERR: can not build data connection", "buildstream");
            return std::move(session);
        }
        session.listen(1);
        scmd.sendmsg("PORT " + std::to_string(session.getlocaladdr().port));

        Session dsession = session.accept(5, ACTIVE);
        if(!dsession.status()) {
            logger("ERR: can not build data connection", "buildstream");
            return std::move(dsession);
        }

        // 建立会话
        return std::move(dsession);
    }
}

bool login(Session& scmd, string user, string passwd) {
    scmd.sendmsg("USER " + user);
    if(!scmd.expect("OK")) return false;
    scmd.sendmsg("PASSWORD " + passwd);
    if(!scmd.expect("OK")) return false;
    return true;
}

void list(Session& scmd) {
    if(!scmd.status()) {
        cout << "not connected" << endl;
        return ;
    }
    string path;
    gettok(path);
    if(path.empty()) path = "."; 
    scmd.sendmsg("LIST " + path);
    if(!scmd.expect("OK")) {
        cout << "request refused" << endl;
        return ;
    }
    Session datasession = buildstream(scmd, mode);
    if(!scmd.expect("BEGIN")) {
        cout << "can not build data session" << endl;
        return ;
    }
    if(!datasession.status()) {
        cout << "error ocur!" << endl;
        return ;
    }
    stringstream listinfo;
    datasession.recvstream(cout);
    cout << endl;
}

void gettime(Session& scmd) {
    if(!scmd.status()) {
        cout << "not connected" << endl;
        return ;
    }
    scmd.sendmsg("TIME");
    string time;
    scmd.recvmsg(time);
    cout << "时间: " << time << endl;
}

void getfile(Session& scmd) {
    if(!scmd.status()) {
        cout << "not connected" << endl;
        return ;
    }
    string target, local;
    while(1) {
        readline(target);
        if(!target.empty()) break;
        cout << "remote target: ";
        nextline();
    }
    cout << "path to save: ";
    nextline();
    readline(local);

    if(fs::exists(local)) {
        cout << "file exist" << endl;
        return ;
    }
    ofstream fout;
    fout.open(local);
    if(!fout) {
        cout << "can not create file" << endl;
        fout.close();
        return ;
    }

    scmd.sendmsg("GET " + target);
    if(!scmd.expect("OK")) {
        cout << "request refused" << endl;
        return ;
    }
    Session datasession = buildstream(scmd, mode);
    if(!datasession.status()) {
        cout << "error ocur!" << endl;
        return ;
    }
    if(!scmd.expect("BEGIN")) {
        cout << "can not build data session" << endl;
        return ;
    }
    datasession.recvstream(fout);
    fout.close();
    cout << "file saved: " + local << endl;
}

void putfile(Session& scmd) {
    if(!scmd.status()) {
        cout << "not connected" << endl;
        return ;
    }
    string target, local;
    while(1) {
        readline(local);
        if(!local.empty()) break;
        cout << "local target: ";
        nextline();
    }
    cout << "remote path to save: ";
    nextline();
    readline(target);

    if(!fs::exists(local)) {
        cout << "path not exisit" << endl;
        return ;
    }
    if(!fs::is_regular_file(local)) {
        cout << "ERR: it is not a file" << endl;
        return ;
    } 

    std::ifstream fin;
    fin.open(local);
    if(!fin) {
        cout << "ERR: can not open file" << endl;
        fin.close();
        return ;
    }
    scmd.sendmsg("PUT " + target);
    if(!scmd.expect("OK")) {
        cout << "request refused" << endl;
        return ;
    }
    Session datasession = buildstream(scmd, mode);
    if(!datasession.status()) {
        cout << "error ocur!" << endl;
        return ;
    }
    if(!scmd.expect("BEGIN")) {
        cout << "can not build data session" << endl;
        return ;
    }
    datasession.sendstream(fin);
    fin.close();
}

void close(Session& scmd) {
    if(scmd.status()) {
        scmd.sendmsg("BYE");
        scmd.close();
    }
}

void sync(Session& scmd) {
    scmd.sendmsg("HELLO");
    while(!scmd.expect("HELLO"));
}

void status(Session& scmd) {
    if(scmd.status()) {
        sync(scmd);
        Ipaddr local = scmd.getlocaladdr();
        Ipaddr target = scmd.gettargetaddr();
        cout << "connecting" << endl;
        cout << "[local]  " << local.getaddr() << ":" << local.port << endl;
        cout << "[target] " << target.getaddr() << ":" << target.port << endl;
    } else {
        cout << "not connected" << endl;
    }
}

void open(Session& scmd) {
    if(scmd.status()) {
        cout << "already connected" << endl;
    } else {
        string ip;
        while(1) {
            gettok(ip);
            if(!ip.empty()) break;
            cout << "target [IP:PORT] : ";
            nextline();
        }
        Ipaddr target;
        if(!parseIp(target, ip)) {
            cout << "can not parse address" << endl;
            return ;
        }
        if(!scmd.starttargetsession(target, ACTIVE)) {
            cout << "can not locate target" << endl;
            return ;
        }
        if(!scmd.expect("LOGIN")) {
            cout << "unexpect situation" << endl;
            return ;
        }
        string user, passwd;
        cout << "USER: ";
        nextline();
        gettok(user);
        cout << "PASSWORD: ";
        nextline();
        gettok(passwd);
        if(login(scmd, user, passwd)) {
            cout << "open success" << endl;
            return ;
        } else {
            cout << "login failed" << endl;
            return ;
        }
    }
}


void runcmd(Session& scmd) {
    if(!scmd.status()) {
        cout << "not connected" << endl;
        return ;
    }
    string cmd;
    while(1) {
        readline(cmd);
        if(!cmd.empty()) break;
        cout << "cmd: ";
        nextline();
    }
    scmd.sendmsg("RUN " + cmd);
    if(!scmd.expect("OK")) {
        cout << "error ocr" << endl;
        return ;
    }
    string result;
    scmd.recvmsg(result);
    cout << result << endl;
}

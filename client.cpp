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

/**********************SYSTEM COMMAND***************************/

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
    ,"debug"        , "打开/关闭调试开关"
    ,"!"            , "运行本地指令"
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


/*********************USER COMMAND***********************/

#define PRETIPSINPUT(TIPS, OBJ) \
    while(1) { \
        readline(OBJ);\
        if(!OBJ.empty()) break;\
        cout << TIPS;\
        nextline();\
    }

#define TIPSINPUT(TIPS, OBJ) \
    cout << TIPS; \
    nextline(); \
    readline(OBJ);

#define EXPECT(EXP, MSG) \
    if(!scmd.expect(EXP)) { \
        cout << MSG << endl; \
        return ; \
    }

#define MUSTNOT(EXP, MSG, OP) \
    if(EXP) {\
        cout << MSG << endl; \
        OP; \
        return ; \
    }

#define MUST(EXP, MSG, OP) \
    if(!(EXP)) {\
        cout << MSG << endl; \
        OP; \
        return ; \
    }

void list(Session& scmd) {
    MUST(scmd.status(), "not connected",);
    string path;
    gettok(path);
    if(path.empty()) path = "."; 
    scmd.sendmsg("LIST " + path);
    EXPECT("OK", "request refused");
    
    Session datasession = buildstream(scmd, mode);
    MUST(datasession.status(), "error ocur!", );
    EXPECT("BEGIN", "can not build data session");
    stringstream listinfo;
    datasession.recvstream(cout);
    cout << endl;
}

void gettime(Session& scmd) {
    MUST(scmd.status(), "not connected",);
    scmd.sendmsg("TIME");
    string time;
    scmd.recvmsg(time);
    cout << "时间: " << time << endl;
}

void getfile(Session& scmd) {
    MUST(scmd.status(), "not connected",);
    string remote, local;
    PRETIPSINPUT("remote target", remote);
    TIPSINPUT("locate path to save: ", local);
    MUSTNOT(fs::exists(local), "file exist", );
    
    ofstream fout;
    fout.open(local);
    MUST(fout, "can not create file", fout.close());

    scmd.sendmsg("GET " + remote);
    EXPECT("OK", "request refused");
    
    Session datasession = buildstream(scmd, mode);
    MUST(datasession.status(), "error ocur!", );
    EXPECT("BEGIN", "can not build data session");
    
    datasession.recvstream(fout);
    fout.close();
    cout << "file saved: " + local << endl;
}

void putfile(Session& scmd) {
    MUST(scmd.status(), "not connected",);
    string remote, local;
    PRETIPSINPUT("local target: ", local);
    TIPSINPUT("remote path to save: ", remote);

    MUST(fs::exists(local), "path not exisit", );
    MUST(fs::is_regular_file(local), "ERR: it is not a file", );

    std::ifstream fin;
    fin.open(local);
    MUST(fin, "ERR: can not open file", fin.close());
    
    scmd.sendmsg("PUT " + remote);
    EXPECT("OK", "request refused");
    Session datasession = buildstream(scmd, mode);
    MUST(datasession.status(), "error ocur!", );
    EXPECT("BEGIN", "can not build data session");
    // 获得文件大小
    fin.seekg(0, fin.end);
    int fsize = fin.tellg();
    fin.seekg(0, fin.beg);

    datasession.sendstream(fin, fsize);
    fin.close();
}

void open(Session& scmd) {
    MUSTNOT(scmd.status(), "already connected", );
    string ip;
    Ipaddr remote;
    PRETIPSINPUT("remote [IP:PORT] : ", ip);
    MUST(parseIp(remote, ip), "can not parse address", );
    MUST(scmd.starttargetsession(remote, ACTIVE), "can not locate target", );
    EXPECT("LOGIN", "unexpect situation");
    
    string user, passwd;
    
    TIPSINPUT("USER: ", user);
    TIPSINPUT("PASSWORD: ", passwd);
    if(login(scmd, user, passwd)) {
        cout << "open success" << endl;
        return ;
    } else {
        cout << "login failed" << endl;
        return ;
    }
}
void runlocalcmd() {
    string cmd;
    readline(cmd);
    system(cmd.c_str());
}

void runcmd(Session& scmd) {
    MUST(scmd.status(), "not connected",);
    string cmd;
    PRETIPSINPUT("cmd: ", cmd);
    scmd.sendmsg("RUN " + cmd);
    EXPECT("OK", "error ocr");
    string result;
    scmd.recvmsg(result);
    cout << result << endl;
}

#include "client.h"
#include <algorithm>
#include <iomanip>
#include <vector>
#include <fstream>
#include <sstream>
#include "utils.h"
#include "socket.h"

CONNECTMODE mode = PASV;


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

std::vector<std::string> cmds = {
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
    ,"mode"         , "切换数据连接方式（PASV：被动 ACTV：主动）"
    ,"!"            , "运行本地指令"
};

void printhelp() {
    std::string cmd;
    gettok(cmd);
    if(cmd.empty()) {
        std::cout << "输入help [cmd]查看帮助" << std::endl;
        for(int i = 0; i < cmds.size(); i += 2) {
            std::cout << std::setw(10) << cmds[i];
            if((i / 2 + 1) % 4 == 0) std::cout << std::endl;
        }
        std::cout << std::endl;
    } else {
        bool found = false;
        for(int i = 0; i < cmds.size(); i+= 2) {
            if(cmds[i] == cmd) {
                std::cout << cmd << ": " << cmds[i + 1] << std::endl;
                found = true;
                break;
            }
        }
        if(!found) std::cout << cmd << ": " << "找不到指令" << std::endl;
    }
}

Session buildstream(Session& scmd, CONNECTMODE mode) {
    std::string cmd;
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
        
        if(!session.listen(1)) {
            logger("ERR: can not build data connection", "buildstream");
            return std::move(session);
        }
        
        scmd.sendmsg("PORT " + std::to_string(session.getlocaladdr().port));

        Session dsession = session.accept(ACTIVE, ACPTTIMEOUT);
        if(!dsession.status()) {
            logger("ERR: can not build data connection", "buildstream");
            return std::move(dsession);
        }
        // 建立会话
        return std::move(dsession);
    }
}

bool login(Session& scmd, std::string user, std::string passwd) {
    scmd.sendmsg("USER " + user);
    if(!scmd.expect("OK")) return false;
    scmd.sendmsg("PASSWORD " + passwd);
    if(!scmd.expect("OK")) return false;
    return true;
}

void close(Session& scmd) {
    if(scmd.status()) {
        scmd.sendmsg("BYE");
        // 先关闭连接，再等待对方关闭连接
        scmd.close();
    }
}

void sync(Session& scmd) {
    if(!scmd.status()) return ;
    scmd.sendmsg("HELLO");
    while(scmd.status() && !scmd.expect("HELLO"));
}


void status(Session& scmd) {
    if(scmd.status()) {
        sync(scmd);
        Ipaddr local = scmd.getlocaladdr();
        Ipaddr target = scmd.gettargetaddr();
        std::cout << "connecting" << std::endl;
        std::cout << "[local]  " << local.getaddr() << ":" << local.port << std::endl;
        std::cout << "[target] " << target.getaddr() << ":" << target.port << std::endl;
    } else {
        std::cout << "not connected" << std::endl;
    }
}



#define PRETIPSINPUT(TIPS, OBJ) \
    while(1) { \
        readline(OBJ);\
        if(!OBJ.empty()) break;\
        std::cout << TIPS;\
        nextline();\
    }

#define TIPSINPUT(TIPS, OBJ) \
    std::cout << TIPS; \
    nextline(); \
    readline(OBJ);

#define EXPECT(EXP, MSG) \
    if(!scmd.expect(EXP)) { \
        std::cout << MSG << std::endl; \
        return ; \
    }

#define MUSTNOT(EXP, MSG, OP) \
    if(EXP) {\
        std::cout << MSG << std::endl; \
        OP; \
        return ; \
    }

#define MUST(EXP, MSG, OP) \
    if(!(EXP)) {\
        std::cout << MSG << std::endl; \
        OP; \
        return ; \
    }

void list(Session& scmd) {
    MUST(scmd.status(), "not connected",);
    std::string path;
    gettok(path);
    if(path.empty()) path = "."; 
    scmd.sendmsg("LIST " + path);
    EXPECT("OK", "request refused");
    
    Session datasession = buildstream(scmd, mode);
    MUST(datasession.status(), "error ocur!", );
    EXPECT("BEGIN", "can not build data session");
    std::stringstream listinfo;
    datasession.recvstream(std::cout);
    std::cout << std::endl;
}

void gettime(Session& scmd) {
    MUST(scmd.status(), "not connected",);
    scmd.sendmsg("TIME");
    std::string time;
    scmd.recvmsg(time);
    std::cout << "TIME: " << time << std::endl;
}

void getfile(Session& scmd) {
    MUST(scmd.status(), "not connected",);
    std::string remote, local;
    PRETIPSINPUT("remote target", remote);
    TIPSINPUT("locate path to save: ", local);
    MUSTNOT(fs::exists(local), "file exist", );
    
    std::ofstream fout;
    fout.open(local, std::ios::binary);
    MUST(fout, "can not create file", fout.close());
    // 发送请求
    scmd.sendmsg("GET " + remote);
    // 期望接收到OK，如果没接收到，打印错误信息并退出
    EXPECT("OK", "request refused");
    // 建立连接
    Session datasession = buildstream(scmd, mode);
    MUST(datasession.status(), "error ocur!", );
    EXPECT("BEGIN", "can not build data session");
    // 接收
    datasession.recvstream(fout);
    fout.close();
    std::cout << "file saved: " + local << std::endl;
}

void putfile(Session& scmd) {
    MUST(scmd.status(), "not connected",);
    std::string remote, local;
    PRETIPSINPUT("local target: ", local);
    TIPSINPUT("remote path to save: ", remote);

    MUST(fs::exists(local), "path not exisit", );
    MUST(fs::is_regular_file(local), "ERR: it is not a file", );

    std::ifstream fin;
    // 要以二进制方式传输，不然可能造成传输中断
    fin.open(local, std::ios::binary);
    MUST(fin, "ERR: can not open file", fin.close());
    
    scmd.sendmsg("PUT " + remote);
    EXPECT("OK", "request refused");
    Session datasession = buildstream(scmd, mode);
    MUST(datasession.status(), "error ocur!", );
    EXPECT("BEGIN", "can not build data session");
    // 获得文件大小
    fin.seekg(0, fin.end);
    size_t fsize = fin.tellg();
    fin.seekg(0, fin.beg);

    datasession.sendstream(fin, fsize);
    fin.close();
}

void open(Session& scmd) {
    MUSTNOT(scmd.status(), "already connected", );
    std::string ip;
    Ipaddr remote;
    PRETIPSINPUT("remote [IP:PORT] : ", ip);
    MUST(parseIp(remote, ip), "can not parse address", );
    MUST(scmd.starttargetsession(remote, ACTIVE), "can not locate target", );
    EXPECT("LOGIN", "unexpect situation");
    
    std::string user, passwd;
    
    TIPSINPUT("USER: ", user);
    TIPSINPUT("PASSWORD: ", passwd);
    if(login(scmd, user, passwd)) {
        std::cout << "open success" << std::endl;
        return ;
    } else {
        std::cout << "login failed" << std::endl;
        return ;
    }
}
void runlocalcmd() {
    std::string cmd;
    readline(cmd);
    system(cmd.c_str());
}

void runcmd(Session& scmd) {
    MUST(scmd.status(), "not connected",);
    std::string cmd;
    PRETIPSINPUT("cmd: ", cmd);
    scmd.sendmsg("RUN " + cmd);
    EXPECT("OK", "error ocr");
    std::string result;
    scmd.recvmsg(result);
    std::cout << result << std::endl;
}

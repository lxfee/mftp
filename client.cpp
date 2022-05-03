#include "client.h"

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

bool login(Session& scmd, std::string user, std::string passwd) {
    scmd.sendmsg("USER " + user);
    if(!scmd.expect("OK")) return false;
    scmd.sendmsg("PASSWORD " + passwd);
    if(!scmd.expect("OK")) return false;
    return true;
}
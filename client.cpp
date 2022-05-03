#include "client.h"



bool login(Session& scmd, std::string user, std::string passwd) {
    scmd.sendmsg("USER " + user);
    if(!scmd.expect("OK")) return false;
    scmd.sendmsg("PASSWORD " + passwd);
    if(!scmd.expect("OK")) return false;
    return true;
}
// TODO 解决断开后无法重新连接
#include "logger.hpp"
#include <string>
#include "session.h"

enum  CONNECTMODE {PASV, ACTV};

bool login(Session& scmd, std::string user, std::string passwd);
Session buildstream(Session& scmd, CONNECTMODE mode);

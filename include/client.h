#pragma once
#include "logger.hpp"
#include <string>
#include "session.h"

enum  CONNECTMODE {PASV, ACTV};

Session buildstream(Session& scmd, CONNECTMODE mode);

bool login(Session& scmd, std::string user, std::string passwd);

void printhelp();

void list(Session& scmd);

void gettime(Session& scmd);

void getfile(Session& scmd);

void putfile(Session& scmd);

void runcmd(Session& scmd);

void close(Session& scmd);

void sync(Session& scmd);

void status(Session& scmd);

void open(Session& scmd);

void runlocalcmd();

extern std::string buffer;
void gettok(std::string& cmd);
void readline(std::string& line);
void nextline();
bool excpet(std::string exp);

#ifndef API_H
#define API_H

#pragma once

#include <stdlib.h>

#define SOCKNAME "../mysock"
#define UNIX_PATH_MAX 108



int sock;
//connect to the server
int openConnection(const char* sockname, int msec, const struct timespec abstime);
int closeConnection();

#endif
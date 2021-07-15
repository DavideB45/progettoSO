#ifndef SERVER_H
#define SERVER_H

#pragma once
#define DEFAULT_SOCK_NAME "../mysock"
#define UNIX_PATH_MAX 108

#define S_WORKING 0
#define S_SLOW_CLOSE 1
#define S_FAST_CLOSE 2


typedef struct ServerInfo{
	char* sockName;
	int sockFD;
	int serverStatus;
	int n_worker;
}ServerInfo;


#endif
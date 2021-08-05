#ifndef SERVER_H
#define SERVER_H

#pragma once

#include <FifoList.h>

#define DEFAULT_SOCK_NAME "../mysock"
#define UNIX_PATH_MAX 108

// macro per informare il dispatcher della 
// chiusura di un socket ricevuto sulla pipe
#define NOT_CONNECTED 0
#define CONNECTED 1
#define CONN_MARK(X, Y) X = ((X << 1) | (Y & 1))
#define GET_FD(X) X>>1
#define IS_TO_RESET(X) X % 2 == 1

#define S_WORKING 0
#define S_SLOW_CLOSE 1
#define S_FAST_CLOSE 2


typedef struct ServerInfo{
	char* sockName;
	char* logName;
	int sockFD;
	int maxFileNum;
	int maxFileDim;
	int doneReq[2];
	FifoList* toServe;
	int serverStatus;
	int n_worker;
}ServerInfo;


#endif
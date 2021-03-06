#ifndef LOGFUN_H
#define LOGFUN_H

#pragma once

#include <request.h>
#include <time.h>

#define CLIENT_OP(x) (x->client > 0)
// visione replace
#define LRU_ALG -1
#define LRU_REPLACE(x) (x->client == LRU_ALG)
#define FILE_REMOVED(x) x->dimReturn
#define FREED_MEMORY(x) x->deltaDim
// visione worker
#define WORKER -3
#define WORKER_ERR(x) (x->client == WORKER)
// visione chiusura
#define SERVER -4
#define SERV_CLOSE(x) (x->client == SERVER)
#define FILE_REMANING(x) x->dimReturn
#define SPACE_USE(x) x->deltaDim
// tipi di apertura
#define LOCK_OPEN 1
#define CREA_OPEN 2
#define LOCR_OPEN 3
#define WITH_LOCK(x) ((x->deltaDim & LOCK_OPEN) != 0)
#define WITH_CREA(x) ((x->deltaDim & CREA_OPEN) != 0)

typedef struct logOp{
	enum operat opType;
	time_t execTime;
	char* fileName;
	int client;
	int result;
	int thread;
	int deltaDim;
	int dimReturn;
}LogOp;

LogOp* newLogOp(enum operat opType, char* fileName, int client, int thread, int result, int deltaDim, int dimReturn);

void destroyLogOp(void* oper);






#endif
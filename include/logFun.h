#ifndef LOGFUN_H
#define LOGFUN_H

#pragma once

#include <request.h>
#include <time.h>

#define CLIENT_OP(x) (x->client > 0)
// visione replace
#define LRU_REPLACE(x) (x->client == -1)
#define FILE_REMOVED(x) x->dimReturn
#define FREED_MEMORY(x) x->deltaDim
// visione max
#define LRU_NEW_MAX(x) (x->client == -2)
#define NEW_MAX_TIPE(x) x->result
#define MAXDIM 0
#define MAXFILE 1
#define MAXALL 2
#define NEW_DIM(x) x->deltaDim
#define NEW_FILE_N(x) x->dimReturn
// visione chiusura
#define SERV_CLOSE(x) (x->client == -3)
#define FILE_REMANING(x) x->dimReturn
#define SPACE_USE(x) x->deltaDim
// alla fine bisogna stampare anche l'albero

typedef struct logOp{
	enum operat opType;
	time_t execTime;
	char* fileName;
	int client;
	int result;
	int deltaDim;
	int dimReturn;
}LogOp;

LogOp* newLogOp(enum operat opType, const char* fileName, int client, int result, int deltaDim, int dimReturn);

void destroyLogOp(LogOp* oper);






#endif
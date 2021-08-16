#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <logFun.h>
#include <request.h>

LogOp* newLogOp( enum operat opType, const char* fileName, int client, int result, int deltaDim, int dimReturn){
	LogOp* new = malloc(sizeof(LogOp));
	if(new == NULL){
		errno = ENOMEM;
		return NULL;
	}
	if(fileName != NULL){
		int nLen = strlen(fileName) + 1;
		new->fileName = malloc(nLen*sizeof(char));
		if(new->fileName == NULL){
			free(new);
			errno = ENOMEM;
			return NULL;
		}
		new->fileName = strcpy(new->fileName, fileName);
	} else {
		new->fileName = NULL;
	}
	new->opType = opType;
	new->client = client;
	new->result = result;
	new->deltaDim = deltaDim;
	new->dimReturn = dimReturn;
	new->execTime = time(NULL);
	return new;
}

void destroyLogOp(LogOp* oper){
	if(oper->fileName != NULL){
		free(oper->fileName);
	}
	free(oper);
}
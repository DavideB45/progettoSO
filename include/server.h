#ifndef SERVER_H
#define SERVER_H

#pragma once

#include <FifoList.h>
#include <files.h>
#include <pthread.h>

#define DEFAULT_SOCK_NAME "../mysock"
#define UNIX_PATH_MAX 108

#define FALSE 0
#define TRUE 1

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

enum operResult{
	FAILED_CONT,// fallita ma il worker puo' continuare la gestione del file
	COMPLETED_CONT,// eseguita con successo, il worker puo' continuare
	FAILED_STOP,// DELAIED  MISSING_FILE
	COMPLETED_STOP,// LOCK
	FILE_DELETED,// file cancellato dall'op, il worker informa altre richieste
	DELAYED,
	NONE// indica operazioni che non riguardano un singolo file
};

typedef struct trhreadInfo{
	pthread_t thread;
	pthread_mutex_t lock;// per accedere al filePtr
	pthread_cond_t completedReq;// signal quando finisce l'operazione
	ServerFile* filePtr;// su cosa opera
}ThreadInfo;


typedef struct ServerInfo{
	char* sockName;// nome del socket
	char* logName;// nome del file di log
	int sockFD;// file descriptor del socket
	int maxFileNum;
	int maxFileDim;
	int doneReq[2];// pipe per informare dispatcher
	FifoList* toServe;// richieste dei client
	int serverStatus;// per capire se deve chiudere
	int n_worker;// totale thread worker
	ThreadInfo* *threadUse;// array che dice cosa fanno i thread
}ServerInfo;


#endif
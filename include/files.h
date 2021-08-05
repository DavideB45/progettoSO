#ifndef FILES_H
#define FILES_H

#pragma once

#include <stdlib.h>
#include <pthread.h>
#include <generalList.h>
#include <request.h>

typedef struct ServerFile{
	pthread_mutex_t lock;
	_Bool flagUse;
	_Bool flagO_lock;
	int lockOwner;// cliend ID = Inode del coso per prlarci
	GeneralList* openList;// 
	GeneralList* requestList;// riempita da atri thread
	int dim;//capire quale deve essere l'unita' di misura
	char* data;
	int creator;
}ServerFile;


// funzione che ritorna 0;
int fakeComp(const void* a,const void* b);

// crea un nuovo file per il server
// ritorna NULL se fallisce
ServerFile* newServerFile(int creator, int O_lock);

// distruggr il file
// setta obj = NULL
void destroyServerFile(ServerFile* obj);

//se obj != NULL setta flag O_Lock
//fallisce se la lock e' gia' settata
//ritorna 1 successo 0 altrimenti
int lockFile(ServerFile* obj, int locker);

//resetta il flag O_lock se locker e' il possessore della lock
//ritorna 1 successo ritorna 0 altrimenti
int unlockFile(ServerFile* obj, int locker);

//da migliorare
int startMutex(ServerFile* obj);

//da mogliorare
int endMutex(ServerFile* obj);

//aggiunge una richiersta alla coda delle richieste del file
//ritorna 1 successo ritorna numeri > 1 altrimenti
int addRequest(ServerFile *obj, Request* richiesta);

//ritorna un puntatore ad un'operazione se ce ne sono da eseguire
//ritorna null altrimenti
Request* readRequest(ServerFile *obj);

#endif
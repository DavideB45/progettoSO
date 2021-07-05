#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../utils/utils.c"
#include "../utils/generalList.c"


int fakeComp(void* a, void* b){
	printf("stai usando una funzione non implementata\n");
	return 0;
}

typedef struct Request{
	int type;
	int client;
	/* elem x eseguirla */
}Request;

typedef struct ServerFile{
	pthread_mutex_t lock;
	_Bool flagUse;
	_Bool flagO_lock;
	int lockOwner;// cliend ID = Inode del coso per prlarci
	IntList* openList;// 
	IntList* requestList;// riempita da atri thread
	int dim;//capire quale deve essere l'unita' di misura
	char* data;
}ServerFile;

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// SERVER FILE ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

ServerFile* newServerFile(int creator, int O_lock){
	if(creator < 0){
		//errore parametri
		printf("creator < 0\n");
		return NULL;
	}
	
	ServerFile *newFile = malloc(sizeof(ServerFile));
	if(newFile == NULL){
		//non ho creato newFile
		perror("mallock ServerFile");
		return NULL;
	}
	
	newFile->data = NULL;
	newFile->dim = 0;
	newFile->flagUse = 1;
	
	if(O_lock != 0){
		newFile->lockOwner = creator;
		newFile->flagO_lock = 1;
	} else {
		newFile->lockOwner = -1;
		newFile->flagO_lock = 0;
	}
	
	if(pthread_mutex_init( &(newFile->lock), NULL )){
		pthread_mutex_destroy( &(newFile->lock) );
		free(newFile);
		return NULL;
	}
	
	newFile->openList = newIntList(intCompare, free);
	if (newFile->openList == NULL){
		//non ho creato openList
		pthread_mutex_destroy( &(newFile->lock) );
		free(newFile);
		return NULL;
	}
	intListInsert(creator, newFile->openList);
	
	//devo modificare la struttura
	newFile->requestList = newIntList(fakeComp, free);
	if(newFile->requestList == NULL){
		//non ho creato requestList
		pthread_mutex_destroy( &(newFile->lock) );
		free(newFile);
		intListDestroy(newFile->openList);
		return NULL;
	}
	return newFile;
}

void destroyServerFile(ServerFile* obj){
	pthread_mutex_destroy( &(obj->lock) );
	intListDestroy(obj->openList);
	intListDestroy(obj->requestList);
	free(obj->data);
	free(obj);
	return;
}

/*void declareUse(ServerFile *obj){
	Pthread_mutex_lock( &(obj->lock) );
		obj->flagUse = 1;
	Pthread_mutex_unlock( &(obj->lock) );
}
void stopUse(ServerFile *obj){
	Pthread_mutex_lock( &(obj->lock) );
		obj->flagUse = 0;
	Pthread_mutex_unlock( &(obj->lock) );
}
*/

void  lockFile(ServerFile* obj, int locker){	
	obj->flagO_lock = 1;
	obj->lockOwner = locker;
	return;
}

_Bool unlockFile(ServerFile* obj, int locker){
	if(locker == obj->lockOwner){
		obj->flagO_lock = 0;
		obj->lockOwner = locker;
		return 1;
	} else {
		return 0;
	}
}

void startMutex(ServerFile* obj){
	Pthread_mutex_lock( &(obj->lock) );
}

void endMutex(ServerFile* obj){
	Pthread_mutex_unlock( &(obj->lock) );
}

int addRequest(ServerFile *obj, Request* richiesta){
	if(obj == NULL){
		return 2;
	}
	if(richiesta == NULL){
		return 3;
	}
	if( intListInsert(richiesta ,obj->requestList ) ){
		return 0;
	} else {
		return 4;
	}
}

Request* readRequest(ServerFile *obj){
	return ( Request* ) intListPop(obj->requestList);
}
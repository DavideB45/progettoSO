#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <utils.h>
#include <errno.h>
#include <generalList.h>
#include <files.h>
// #include "../../include/files.h"

int fakeComp(void* a, void* b){
	printf("stai usando una funzione non implementata\n");
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// SERVER FILE ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

ServerFile* newServerFile(int creator, int O_lock){
	if(creator < 0){
		//errore parametri
		printf("creator < 0\n");
		errno = EFAULT;
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
	
	newFile->openList = newGeneralList(intCompare, free);
	if (newFile->openList == NULL){
		//non ho creato openList
		pthread_mutex_destroy( &(newFile->lock) );
		free(newFile);
		return NULL;
	}
	generalListInsert(creator, newFile->openList);
	
	//devo modificare la struttura
	newFile->requestList = newGeneralList(fakeComp, free);
	if(newFile->requestList == NULL){
		//non ho creato requestList
		pthread_mutex_destroy( &(newFile->lock) );
		free(newFile);
		generalListDestroy(newFile->openList);
		return NULL;
	}
	return newFile;
}

void destroyServerFile(ServerFile* obj){
	if(obj == NULL)
		return;
	
	pthread_mutex_destroy( &(obj->lock) );
	generalListDestroy(obj->openList);
	generalListDestroy(obj->requestList);
	free(obj->data);
	free(obj);
	obj = NULL;
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

int  lockFile(ServerFile* obj, int locker){	
	if(obj == NULL){
		errno = EFAULT;
		return 0;
	}
	if(obj->flagO_lock == 1){
		return 0;
	}
	
	obj->flagO_lock = 1;
	obj->lockOwner = locker;
	return 1;
}

int unlockFile(ServerFile* obj, int locker){
	if(obj == NULL){
		errno = EFAULT;
		return 0;
	}
	
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
		errno = EINVAL;
		return 2;
	}
	if(richiesta == NULL){
		errno = EINVAL;
		return 3;
	}
	if( generalListInsert(richiesta ,obj->requestList ) ){
		return 1;
	} else {
		return 4;
	}
}

Request* readRequest(ServerFile *obj){
	if(obj == NULL){
		errno = EFAULT;
		return NULL;
	}
	return ( Request* ) generalListPop(obj->requestList);
}
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>
#include <errno.h>

#include <pthread.h>
#include <string.h>

#include <generalList.h>
#include <files.h>
#include <request.h>

int fakeComp(const void* a,const void* b){
	printf("stai usando una funzione non implementata\n");
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// SERVER FILE ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

ServerFile* newServerFile(int creator, int O_lock, char* nameP){
	if(creator < 0 || nameP == NULL){
		//errore parametri
		printf("creator < 0 || name P == NULL\n");
		errno = EFAULT;
		return NULL;
	}
	

	ServerFile *newFile = malloc(sizeof(ServerFile));
	if(newFile == NULL){
		//non ho creato newFile
		perror("mallock ServerFile");
		errno = ENOMEM;
		return NULL;
	}
	
	if(O_lock){
		newFile->creator = creator;
	} else {
		newFile->creator = -1;
	}
	
	int nameL = strlen(nameP) + 1;
	newFile->namePath = malloc(nameL*sizeof(char));
	if(newFile->namePath == NULL){
		perror("mallock ServerFile->name");
		errno = ENOMEM;
		return NULL;
	}
	newFile->namePath = strcpy(newFile->namePath, nameP);
	newFile->data = NULL;
	newFile->dim = 0;

	
	
	if(O_lock != 0){
		newFile->lockOwner = creator;
		newFile->flagO_lock = 1;
		newFile->flagUse = 0;// le richieste successive saranno trascurate
	} else {
		newFile->lockOwner = -1;
		newFile->flagO_lock = 0;
		newFile->flagUse = 1;// poi continuero' a eseguire richieste sul file
	}
	
	
	newFile->openList = newGeneralList( intCompare, free);
	if (newFile->openList == NULL){
		//non ho creato openList
		free(newFile);
		errno = ENOMEM;
		return NULL;
	}
	int* newOpen = malloc(sizeof(int));
	if(newOpen == NULL){
		generalListDestroy(newFile->openList);
		free(newFile);
		errno = ENOMEM;
		return NULL;
	}
	*newOpen = creator;
	generalListInsert( (void*) newOpen, newFile->openList);
	
	//devo modificare la struttura
	newFile->requestList = newGeneralList(fakeComp, free);
	if(newFile->requestList == NULL){
		//non ho creato requestList
		free(newOpen);
		free(newFile);
		generalListDestroy(newFile->openList);
		errno = ENOMEM;
		return NULL;
	}	
	errno = 0;
	return newFile;
}

void destroyServerFile(ServerFile* obj){
	if(obj == NULL)
		return;
	
	generalListDestroy(obj->openList);
	generalListDestroy(obj->requestList);
	free(obj->namePath);
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
		return obj->lockOwner == locker;
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
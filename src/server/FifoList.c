#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <utils.h>
#include <FifoList.h>
#include <errno.h>
// #include "../../include/utils.h"
// #include "../../include/FifoList.h"


// create an empty list
FifoList* newList(){
	FifoList *list = (FifoList*) malloc(sizeof(FifoList));
	if(list == NULL){
		perror("list malloc");
		return NULL;
	}
	list->dim = 0;
	list->headPtr = NULL;
	list->queuePtr = NULL;
	if( Pthread_mutex_init( &(list->lock) ) != 0){
		free(list);
		return NULL;
	}
	if(pthread_cond_init(&(list->wait_to_read), NULL) != 0){
		pthread_mutex_destroy( &(list->lock) );
		free(list);
		return NULL;
	}
	return list;
}

// return 1 if empty 0 if not empty
// -1 error
int isEmpty(FifoList* list){
	if(list == NULL){
		return -1;
	}
	
	return list->dim == 0;
}

// insert in queue
// ret -> 0 failure 1 success
_Bool insert(FifoList* list, void* elem){
	if(list == NULL || elem == NULL){
		return 0;
	}
	
	if( Pthread_mutex_lock(&(list->lock)) != 0){
		return 0;
	}
		FifoNode* newNode = malloc(sizeof(FifoNode));
		if(newNode == NULL){
			perror("malloc FifoNode");
			return 0;
		}
		(list->dim)++;
		newNode->nextPtr = NULL;
		newNode->data = elem;
		if(list->queuePtr != NULL){
			list->queuePtr->nextPtr = newNode;
		} else {
			list->headPtr = newNode;
		}
		list->queuePtr = newNode;
	pthread_cond_signal( &(list->wait_to_read) );
	Pthread_mutex_unlock( &(list->lock) );
	return 1;
}

//remove first element and return its data pointer
void* pop(FifoList* list){
	if(list == NULL){
		return NULL;
	}
	
	if( Pthread_mutex_lock(&(list->lock)) != 0){
		return NULL;
	}
	while(list->dim <= 0){	
		if( pthread_cond_wait(&(list->wait_to_read), &(list->lock)) == EINVAL){
			Pthread_mutex_unlock( &(list->lock) );
			return NULL;
		}
	}
	
		void* value = list->headPtr->data;
		FifoNode* toFree = list->headPtr;
		(list->dim)--;
		list->headPtr = list->headPtr->nextPtr;
		if(list->headPtr == NULL){
			list->queuePtr = NULL;
		}
		free(toFree);
	
	Pthread_mutex_unlock( &(list->lock) );
	return value;
}

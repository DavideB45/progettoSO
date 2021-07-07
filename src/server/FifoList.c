#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <utils.h>
#include <FifoList.h>
// #include "../../include/utils.h"
// #include "../../include/FifoList.h"


//create an empty list
FifoList* newList(){
	FifoList *list = (FifoList*) malloc(sizeof(FifoList));
	if(list == NULL){
		perror("list malloc");
		return NULL;
	}
	list->dim = 0;
	list->headPtr = NULL;
	list->queuePtr = NULL;
	pthread_mutex_init(&(list->lock), NULL);
	pthread_cond_init(&(list->wait_to_read), NULL);
	return list;
}

//return 1 if empty 0 if not empty
_Bool isEmpty(FifoList* list){
	return list->dim == 0;
}

//insert in queue
_Bool insert(FifoList* list, void* elem){
	Pthread_mutex_lock( &(list->lock) );
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
	Pthread_mutex_lock( &(list->lock) );
	while(list->dim <= 0){	
		pthread_cond_wait(&(list->wait_to_read), &(list->lock));
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

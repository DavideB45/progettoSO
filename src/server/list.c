#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "utils.c"

typedef struct GeneralNode{
	void* data;
	struct GeneralNode *nextPtr;
}GeneralNode;

typedef struct GeneralList{
	int dim;
	pthread_mutex_t lock;
	pthread_cond_t wait_to_read;
	GeneralNode* headPtr;
	GeneralNode* queuePtr;
}GeneralList;

//create an empty list
GeneralList* newList(){
	GeneralList *list = (GeneralList*) malloc(sizeof(GeneralList));
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
_Bool isEmpty(GeneralList* list){
	return list->dim == 0;
}

//insert in queue
_Bool insert(GeneralList* list, void* elem){
	Pthread_mutex_lock( &(list->lock) );
		GeneralNode* newNode = malloc(sizeof(GeneralNode));
		if(newNode == NULL){
			perror("malloc GeneralNode");
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
void* pop(GeneralList* list){
	Pthread_mutex_lock( &(list->lock) );
	while(list->dim <= 0){	
		pthread_cond_wait(&(list->wait_to_read), &(list->lock));
	}
		void* value = list->headPtr->data;
		GeneralNode* toFree = list->headPtr;
		(list->dim)--;
		list->headPtr = list->headPtr->nextPtr;
		if(list->headPtr == NULL){
			list->queuePtr = NULL;
		}
		free(toFree);
	Pthread_mutex_unlock( &(list->lock) );
	return value;
}




#include <stdio.h>
#include <stdlib.h>
#include "utils.c"


typedef struct IntListNode{
	void* elem;
	struct IntListNode *nextPtr;
}IntListNode;

typedef struct IntList{
	IntListNode *head;
	IntListNode *queue;
	int (*compFun)(void*, void*);
	void (*freeFun)(void*);
}IntList;

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// INT LIST //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

//crea una bella lista
IntList *newIntList(int (*comp)(void*, void*), void (*freeF)(void*)){
	IntList *newList = malloc(sizeof(IntList));
	if(newList == NULL){
		perror("malloc IntList");
		return NULL;
	}
	newList->compFun = comp;
	newList->freeFun = freeF;
	newList->head = NULL;
	newList->queue = NULL;
	return newList;
}

//remove first occurrence of num
//1 elem rimosso o non presente
_Bool intListRemove(void* elem, IntList* list){
	if (list == NULL){
		//lista non esistente
		printf("list NULL");
		return 0;
	}
	IntListNode *currPtr = list->head;
	if (list->head == NULL){
		return 1;
	}
	
	IntListNode *precPtr = NULL;
	if(!( list->compFun( elem, (list->head->elem) ) )){
		//elemento rimosso dalla testa
		list->head = list->head->nextPtr;
		if (list->head == NULL){
			list->queue = NULL;
		}
		list->freeFun(currPtr->elem);
		free(currPtr);
		return 1;
	}
	while(currPtr != NULL && !( list->compFun( elem, (currPtr->elem) ) )){
		precPtr = currPtr;
		currPtr = currPtr->nextPtr;
	}
	if (currPtr == NULL){
		//elem non presente
		return 1;
	} else {
		//elemento rimosso
		if (currPtr == list->queue){
			list->queue = currPtr->nextPtr;
		}
		precPtr->nextPtr = currPtr->nextPtr;
		list->freeFun(currPtr->elem);
		free(currPtr);
		return 1;
	}
}

//insert at the list's start
//1 andata a buon fine   0 errore
_Bool intListInsert(void *elem, IntList* list){
	if (list == NULL){
		printf("NULL list\n");
		return 0;
	}
	
	IntListNode* newNode = malloc(sizeof(IntListNode));
	if(newNode == NULL){
		perror("malloc list node");
		return 0;
	}

	if(list->head == NULL){
		//lista vuota
		list->head = newNode;
		list->queue = newNode;
		list->head->nextPtr = NULL;
		list->head->elem = elem;
		return 1;
	}
	
	newNode->nextPtr = NULL;
	newNode->elem = elem;
	list->queue->nextPtr = newNode;
	list->queue = newNode;
	return 1;
}

void* intListPop(IntList* list){
	if (list == NULL){
		printf("lista NULL\n");
		return NULL;
	}
	if (list->head == NULL){
		printf("lista vuota\n");
		return NULL;
	}
	IntListNode* toRemove = list->head;
	void* retVal = toRemove->elem;
	list->head = toRemove->nextPtr;
	free(toRemove);
	if (list->head == NULL){
		list->queue = NULL;
	}
	return retVal;
}

//1 c'e'   0 non c'e'
int isInIntList(void* elem, IntList* list){
	if (list == NULL){
		printf("list NULL\n");
		return -1;
	}
	
	IntListNode* currPtr = list->head;
	while (currPtr != NULL && !( list->compFun(currPtr->elem, elem) ) ){
		currPtr = currPtr->nextPtr;
	}
	return currPtr != NULL;
}

//1 = empty    0 = not empty
int isIntListEmpty(IntList* list){
	if (list == NULL){
		return -1;
	}
	return list->head == NULL;
}

void intListDestroy(IntList* list){
	if (list == NULL){
		return;
	}
	IntListNode* toFree = list->head;
	if (toFree != NULL){
		IntListNode* nextFreePtr = toFree->nextPtr;
		while (nextFreePtr != NULL){
			list->freeFun(toFree->elem);
			free(toFree);
			toFree = nextFreePtr;
			nextFreePtr = nextFreePtr->nextPtr;
		}
		list->freeFun(toFree->elem);
		free(toFree);
	}
	
	free(list);
	list = NULL;
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

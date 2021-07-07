#include <stdio.h>
#include <stdlib.h>
#include "utils.c" 


typedef struct GeneralListNode{
	void* elem;
	struct GeneralListNode *nextPtr;
}GeneralListNode;

typedef struct GeneralList{
	GeneralListNode *head;
	GeneralListNode *queue;
	int (*compFun)(void*, void*);
	void (*freeFun)(void*);
}GeneralList;

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// INT LIST //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

//crea una bella lista
GeneralList *newGeneralList(int (*comp)(void*, void*), void (*freeF)(void*)){
	GeneralList *newList = malloc(sizeof(GeneralList));
	if(newList == NULL){
		perror("malloc GeneralList");
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
_Bool generalListRemove(void* elem, GeneralList* list){
	if (list == NULL){
		//lista non esistente
		printf("list NULL");
		return 0;
	}
	GeneralListNode *currPtr = list->head;
	if (list->head == NULL){
		return 1;
	}
	
	GeneralListNode *precPtr = NULL;
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
_Bool generalListInsert(void *elem, GeneralList* list){
	if (list == NULL){
		printf("NULL list\n");
		return 0;
	}
	
	GeneralListNode* newNode = malloc(sizeof(GeneralListNode));
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

void* generalListPop(GeneralList* list){
	if (list == NULL){
		printf("lista NULL\n");
		return NULL;
	}
	if (list->head == NULL){
		printf("lista vuota\n");
		return NULL;
	}
	GeneralListNode* toRemove = list->head;
	void* retVal = toRemove->elem;
	list->head = toRemove->nextPtr;
	free(toRemove);
	if (list->head == NULL){
		list->queue = NULL;
	}
	return retVal;
}

//1 c'e'   0 non c'e'
int isInGeneralList(void* elem, GeneralList* list){
	if (list == NULL){
		printf("list NULL\n");
		return -1;
	}
	
	GeneralListNode* currPtr = list->head;
	while (currPtr != NULL && !( list->compFun(currPtr->elem, elem) ) ){
		currPtr = currPtr->nextPtr;
	}
	return currPtr != NULL;
}

//1 = empty    0 = not empty
int isGeneralListEmpty(GeneralList* list){
	if (list == NULL){
		return -1;
	}
	return list->head == NULL;
}

void generalListDestroy(GeneralList* list){
	if (list == NULL){
		return;
	}
	GeneralListNode* toFree = list->head;
	if (toFree != NULL){
		GeneralListNode* nextFreePtr = toFree->nextPtr;
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

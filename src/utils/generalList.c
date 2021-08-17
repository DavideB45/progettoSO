#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
// #include "../../include/generalList.h"
// #include "../../include/utils.h"
#include <generalList.h>
#include <utils.h> 



///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// INT LIST //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

//crea una bella lista
GeneralList *newGeneralList(int (*comp)(const void*,const void*), void (*freeF)(void*)){
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

// remove first occurrence of num
// 1 elem rimosso o non presente
// ritorna 0 altrimenti
_Bool generalListRemove(void* elem, GeneralList* list){
	if(list == NULL){
		//lista non esistente
		errno = EFAULT;
		return 0;
	}
	if(elem == NULL){
		// elem non esistente
		errno = EFAULT;
		return 0;
	}
	
	GeneralListNode *currPtr = list->head;
	if (list->head == NULL){
		return 1;
	}
	
	GeneralListNode *precPtr = NULL;
	if(( list->compFun(elem, (list->head->elem)) ) == 0){
		//elemento rimosso dalla testa
		list->head = list->head->nextPtr;
		if (list->head == NULL){
			list->queue = NULL;
		}
		list->freeFun(currPtr->elem);
		free(currPtr);
		return 1;
	}
	while(currPtr != NULL && ( list->compFun( elem, (currPtr->elem) ) ) != 0){
		precPtr = currPtr;
		currPtr = currPtr->nextPtr;
	}
	if (currPtr == NULL){
		//elem non presente
		return 1;
	} else {
		//elemento rimosso
		if (currPtr == list->queue){
			list->queue = precPtr;
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
		errno = EFAULT;
		return 0;
	}
	
	GeneralListNode* newNode = malloc(sizeof(GeneralListNode));
	if(newNode == NULL){
		perror("malloc list node");
		errno = ENOMEM;
		return 0;
	}
	newNode->elem = elem;
	newNode->nextPtr = NULL;
	if(list->head == NULL){
		//lista vuota
		list->head = newNode;
		list->queue = newNode;
		errno = 0;
		return 1;
	}
	
	list->queue->nextPtr = newNode;
	list->queue = newNode;
	errno = 0;
	return 1;
}

void* generalListPop(GeneralList* list){
	if (list == NULL){
		errno = EFAULT;
		return NULL;
	}
	if (list->head == NULL){
		errno = 0;
		return NULL;
	}
	GeneralListNode* toRemove = list->head;
	void* retVal = toRemove->elem;
	list->head = toRemove->nextPtr;
	free(toRemove);
	if (list->head == NULL){
		list->queue = NULL;
	}
	errno = 0;
	return retVal;
}

//1 c'e'   0 non c'e'
int isInGeneralList(void* elem, GeneralList* list){
	if (list == NULL){
		errno = EFAULT;
		return 2;
	}
	if(elem == NULL){
		errno = EFAULT;
		return 3;
	}
	
	GeneralListNode* currPtr = list->head;
	while (currPtr != NULL && ( list->compFun(currPtr->elem, elem) ) != 0 ){
		currPtr = currPtr->nextPtr;
	}
	errno = 0;
	return currPtr != NULL;
}

//1 = empty    0 = not empty
int isGeneralListEmpty(GeneralList* list){
	if (list == NULL){
		errno = EINVAL;
		return -1;
	}
	errno = 0;
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

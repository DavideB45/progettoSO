#ifndef GENERALLIST_H
#define GENERALLIST_H

#pragma once

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


// crea una lista usando i parametri in ingresso
// ritorna NULL in caso di fallimento
GeneralList *newGeneralList(int (*comp)(void*, void*), void (*freeF)(void*));

// remove first occurrence of num
// 1 elem rimosso o non presente
// ritorna 1 se elemento rimosso o non presente 1 altrimenti
_Bool generalListRemove(void* elem, GeneralList* list);

// inserisce un ememento in coda
// 1 andata a buon fine 0 errore
_Bool generalListInsert(void *elem, GeneralList* list);

// ritorna un puntatore al primo oggetto della lista e lo rimuove
// ritorna NULL in caso di lista vuota o di lista = NULL
void* generalListPop(GeneralList* list);

// 1 c'e'   0 non c'e'
// ritorna numeri > 1 in caso di errore
int isInGeneralList(void* elem, GeneralList* list);

// 1 = empty    0 = not empty   -1 = NULL
int isGeneralListEmpty(GeneralList* list);

// distruggela lista
void generalListDestroy(GeneralList* list);

#endif
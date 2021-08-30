#ifndef CLIENTTABLE_H
#define CLIENTTABLE_H

#pragma once

#define BASEDIM 20

#include <generalList.h> 
#include <tree.h>
#include <files.h>

typedef struct _clientInfo{
	pthread_mutex_t lock;
	int id;
	GeneralList* nodeInUse;
	GeneralList* nodeLocked;
	struct _clientInfo* next;
}ClientInfo;

typedef struct clientTable{
	pthread_mutex_t lock;
	ClientInfo* arr[BASEDIM];
}ClientTable;

// crea una ClientTable 
// se fallisce ritorna NULL;
ClientTable* newClientTable();

// libera la memoria occupata da tab;
void destroyClientTable(ClientTable* tab);

// attiva un posto in cui saranno tenute informazioni sul client
// return -1 fail 0 success
int newClient(int clientId, ClientTable *tab);

// mette file nella lista Open
// se non ottengo mutua esclusione non lo metto
// ritorna -1 errore 0 successo
int clientOpen(int clientId, TreeNode* nodePtr, int O_Lock, ClientTable* tab);

// mette file nella lista lock
// se non ottengo mutua esclusione non lo metto
// return -1 fail 0 success setta errno
int clientLock(int clientId, TreeNode* nodePtr, ClientTable* tab);

// chiude tutto quello che aveva aperto (da chiamare alla disconnessione)
// se non ottengo mutua esclusione lascio tutto aperto per chi verra' dopo
// return -1 fail 0 success setta errno
int disconnectClient(int clientId, ClientTable *tab);

// toglie file dalla lista close di clientId
// se non ottengono la mutua esclusione ignoro
// return -1 fail 0 success setta errno
int clientClose(int clientId, TreeNode* nodePtr, int O_Lock, ClientTable* tab);

// toglie file da lista lock
// se non ottengono mutua esclusione ignoro
// return -1 fail 0 success setta errno
int clientUnlock(int clientId, TreeNode* nodePtr, ClientTable* tab);

// dopo la rimozione di un file viene chiuso a tutti per evitare segFault
// da chiamare come ultima funzione di rimozione qunado nessuno ha puntatori a questo file
// return -1 fail 0 success setta errno
int clientFileDel( TreeNode* nodePtr, ClientTable* tab, ServerFile* filePtr);

// restituisce il posto nell'array in cui sta il client
int clientBucket(int clientId);

#endif
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
	// 
	// prima di destroyFlie() nessuno lo puo' avere aperto
	GeneralList* nodeInUse;
	GeneralList* nodeLocked;
	struct _clientInfo* next;
}ClientInfo;

typedef struct clientTable{
	pthread_mutex_t lock;
	ClientInfo* arr[BASEDIM];
}ClientTable;


ClientTable* newClientTable();

// attiva un posto in cui saranno tenute informazioni sul client
// se fallisce chiudo il client
// return -1 fail 0 success
int newClient(int clientId, ClientTable *tab);

// mette file nella lista Open
// se non ottengo mutua esclusione non lo metto
int clientOpen(int clientId, TreeNode* nodePtr, int O_Lock, ClientTable* tab);

// mette file nella lista lock
// se non ottengo mutua esclusione non lo metto
int clientLock(int clientId, TreeNode* nodePtr, ClientTable* tab);



// chiude tutto quello che aveva aperto
// se non ottengo mutua esclusione lascio tutto aperto per chi verra' dopo
int disconnectClient(int clientId, ClientTable *tab);

// toglie file dalla lista close
// se non ottengono la mutua esclusione lo lasciano aperto
// rischio segFault distruggo la struct client o termino
int clientClose(int clientId, TreeNode* nodePtr, int O_Lock, ClientTable* tab);

// toglie file da lista lock
// se non ottengono mutua esclusione 
// rischio segFault chiudo client (il prossimo avra' qualche file gia' locked)
int clientUnlock(int clientId, TreeNode* nodePtr, ClientTable* tab);

// dopo la rimozione di un file viene chiuso a tutti per evitare segFault
int clientFileDel( TreeNode* nodePtr, ClientTable* tab, ServerFile* filePtr);

// restituisce il posto nell'array in cui sta il client
int clientBucket(int clientId);

#endif
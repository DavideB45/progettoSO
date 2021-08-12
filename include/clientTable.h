#ifndef CLIENTTABLE_H
#define CLIENTTABLE_H

#pragma once

#define BASEDIM 20

#include <generalList.h>

typedef struct _clientInfo{
	// una lock
	int id;
	// li aggiorno dopo una remove
	// GeneralList* fileInUse;
	GeneralList* fileLocked;
	struct _clientInfo* next;
}ClientInfo;

typedef struct clientTable{
	ClientInfo* arr[BASEDIM];
}ClientTable;

int newClient(int clientId, ClientTable *tab);

int clientOpen(int client, )

// qunado rimuovi un file come li rimuovi dai file aperti dei client?



#endif
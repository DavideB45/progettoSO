#include <clientTable.h>
#include <generalList.h> 
#include <files.h>
#include <tree.h>
#include <utils.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>


int nodePtrCompare( const void* node1, const void* node2){
	return ((TreeNode*) node1) - ((TreeNode*) node2);
}
void noOp(void* Ptr){
	printf("noOp %p\n", Ptr);
	fflush(stdout);
	return;
}

// ritorna puntatore alla struttura del client
ClientInfo* clientGetNoLock(int clientId, ClientTable *tab){
	int bucket = clientBucket(clientId);
	ClientInfo* toRet = tab->arr[bucket];
	while(toRet != NULL){
		if(toRet->id == clientId){
			return toRet;
		} else {
			toRet = toRet->next;
		}
	}
	return toRet;
}

ClientTable* newClientTable(){
	ClientTable* new = malloc(sizeof(ClientTable));
	if(new == NULL){
		errno = ENOMEM;
		return NULL;
	}
	if( Pthread_mutex_init( &(new->lock) ) != 0){
		errno = EPERM;
		return NULL;
	}
	for(size_t i = 0; i < BASEDIM; i++){
		new->arr[i] = NULL;
	}
	return new;
}

void destroyClientTable(ClientTable * tab){
	if(tab == NULL){
		return;
	}
	ClientInfo* currPtr = NULL;
	ClientInfo* succPtr = NULL;
	for(size_t i = 0; i < BASEDIM; i++){
		if(tab->arr[i] != NULL){
			currPtr = tab->arr[i];
			while(currPtr != NULL){
				succPtr = currPtr->next;
				generalListDestroy(currPtr->nodeInUse);
				generalListDestroy(currPtr->nodeLocked);
				pthread_mutex_destroy( &(currPtr->lock) );
				free(currPtr);
				currPtr = succPtr;
			}
		}
	}
	pthread_mutex_destroy( &(tab->lock) );
	free(tab);
}

// ritorna puntatore alla struttura del client
// poco plausibile che non ci sia
// in caso di errore ritorna NULL
ClientInfo* clientGet(int clientId, ClientTable *tab){
	if(Pthread_mutex_lock( &(tab->lock ) ) != 0){
		errno = EPERM;
		return NULL;
	}
	int bucket = clientBucket(clientId);
	ClientInfo* toRet = tab->arr[bucket];
	while(toRet != NULL){
		if(toRet->id == clientId){
			Pthread_mutex_unlock( &(tab->lock ) );
			errno = 0;
			return toRet;
		} else {
			toRet = toRet->next;
		}
	}
	Pthread_mutex_unlock( &(tab->lock ) );
	errno = 0;
	return toRet;
}

// attiva un posto in cui saranno tenute informazioni sul client
// se fallisce chiudo il client
// return -1 fail 0 success setta errno
int newClient(int clientId, ClientTable *tab){
	if(Pthread_mutex_lock( &(tab->lock ) ) != 0){
		errno = EPERM;
		return -1;
	}
	int bucket = clientBucket(clientId);
	ClientInfo* precClient = NULL;
	ClientInfo* currClient = tab->arr[bucket];
	ClientInfo* new_Client = NULL;
		

	while(currClient != NULL){
		if(currClient->id == clientId){
			Pthread_mutex_unlock( &(tab->lock ) );
			errno = 0;
			return 0;
		}
		currClient = currClient->next;
		precClient = currClient;
	}
	
		new_Client = malloc(sizeof(ClientInfo));
		if(new_Client == NULL){
			Pthread_mutex_unlock( &(tab->lock ) );
			errno = ENOMEM;
			return -1;
		}
		new_Client->id = clientId;
		if(Pthread_mutex_init( &(new_Client->lock) ) != 0){
			free(new_Client);
			Pthread_mutex_unlock( &(tab->lock ) );
			errno = EPERM;
			return -1;
		}
		new_Client->nodeLocked = newGeneralList(nodePtrCompare, noOp);
		if(new_Client->nodeLocked == NULL){
			pthread_mutex_destroy( &(new_Client->lock) );
			free(new_Client);
			Pthread_mutex_unlock( &(tab->lock ) );
			errno = ENOMEM;
			return -1;
		}
		new_Client->nodeInUse = newGeneralList(nodePtrCompare, noOp);
		if(new_Client->nodeInUse == NULL){
			generalListDestroy(new_Client->nodeLocked);
			pthread_mutex_destroy( &(new_Client->lock) );
			free(new_Client);
			Pthread_mutex_unlock( &(tab->lock ) );
			errno = ENOMEM;
			return -1;
		}
		new_Client->next = NULL;

	if(precClient == NULL){
		tab->arr[bucket] = new_Client;
	} else {
		precClient->next = new_Client;
	}
	Pthread_mutex_unlock( &(tab->lock ) );
	errno = 0;
	return 1;
}

// mette file nella lista Open
// se non ottengo mutua esclusione non lo metto
// ritorna -1 errore 0 successo
int clientOpen(int clientId, TreeNode* nodePtr, int O_Lock, ClientTable* tab){
	ClientInfo* clientPtr = clientGet(clientId, tab);
	if(clientPtr == NULL){
		// errno settato da clientGet
		return -1;
	}
	if( Pthread_mutex_lock( &(clientPtr->lock) ) != 0){
		errno = EPERM;
		return -1;
	}
	if(!isInGeneralList((void*) nodePtr, clientPtr->nodeInUse)){
		if(!generalListInsert((void*) nodePtr, clientPtr->nodeInUse)){
			int err = errno;
			Pthread_mutex_unlock( &(clientPtr->lock) );
			errno = err;
			return -1;
		}
	}
	if((O_Lock == 1) && !isInGeneralList((void*) nodePtr, clientPtr->nodeLocked)){
		if(!generalListInsert((void*) nodePtr, clientPtr->nodeLocked)){
			int err = errno;
			generalListRemove((void*) nodePtr, clientPtr->nodeInUse);
			Pthread_mutex_unlock( &(clientPtr->lock) );
			errno = err;
			return -1;
		}
	}
	Pthread_mutex_unlock( &(clientPtr->lock) );
	errno = 0;
	return 0;
}

// mette file nella lista lock
// se non ottengo mutua esclusione non lo metto
// return -1 fail 0 success setta errno
int clientLock(int clientId, TreeNode* nodePtr, ClientTable* tab){
	ClientInfo* clientPtr = clientGet(clientId, tab);
	if(clientPtr == NULL){
		// errno settato da clientGet
		return -1;
	}
	if( Pthread_mutex_lock( &(clientPtr->lock) ) != 0){
		errno = EPERM;
		return -1;
	}
	if(!isInGeneralList((void*) nodePtr, clientPtr->nodeLocked)){
		if(!generalListInsert((void*) nodePtr, clientPtr->nodeLocked)){
			int err = errno;
			Pthread_mutex_unlock( &(clientPtr->lock) );
			errno = err;
			return -1;
		}
	}
	Pthread_mutex_unlock( &(clientPtr->lock) );
	errno = 0;
	return 0;
}



// chiude tutto quello che aveva aperto (da chiamare alla disconnessione)
// se non ottengo mutua esclusione lascio tutto aperto per chi verra' dopo
// return -1 fail 0 success setta errno
int disconnectClient(int clientId, ClientTable *tab){
	ClientInfo* clientPtr = clientGet(clientId, tab);
	if(clientPtr == NULL){
		// errno settato da clientGet
		return -1;
	}
	if( Pthread_mutex_lock( &(tab->lock) ) != 0){
		errno = EPERM;
		return -1;
	}
	if( Pthread_mutex_lock( &(clientPtr->lock) ) != 0){
		Pthread_mutex_unlock( &(tab->lock) );
		errno = EPERM;
		return -1;
	}
	TreeNode* nodePtr = NULL;
	// rimuovo la lock dai file su cui il client aveva la mutua esclusione
	while(nodePtr = generalListPop(clientPtr->nodeLocked), nodePtr != NULL){
		if( Pthread_mutex_lock( &((nodePtr)->lock) ) != 0){
			errno = EPERM;
			return -1;
		}
		(nodePtr)->sFile->flagO_lock = 0;
		generalListRemove(&clientId, (nodePtr)->sFile->openList);
		generalListRemove(nodePtr, clientPtr->nodeInUse);
		Pthread_mutex_unlock( &((nodePtr)->lock) );
	}
	// chiudo i file non chiusi
	while(nodePtr = generalListPop(clientPtr->nodeInUse), nodePtr != NULL){
		if( Pthread_mutex_lock( &((nodePtr)->lock) ) != 0){
			errno = EPERM;
			return -1;
		}
		printf("%d chiudo %p\n", clientId,(void*) nodePtr);
		fflush(stdout);
		generalListRemove(&clientId, (nodePtr)->sFile->openList);
		Pthread_mutex_unlock( &((nodePtr)->lock) );
	}
	Pthread_mutex_unlock( &(clientPtr->lock) );
	Pthread_mutex_unlock( &(tab->lock) );

	errno = 0;
	return 0;
}

// toglie file dalla lista close di clientId
// se non ottengono la mutua esclusione termino / chiudo client e rifaccio struct
// return -1 fail 0 success setta errno
int clientClose(int clientId, TreeNode* nodePtr, int O_Lock, ClientTable* tab){
	ClientInfo* clientPtr = clientGet(clientId, tab);
	if(clientPtr == NULL){
		// errno settato da clientGet
		return -1;
	}
	if( Pthread_mutex_lock( &(clientPtr->lock) ) != 0){
		errno = EPERM;
		return -1;
	}
	generalListRemove(nodePtr, clientPtr->nodeInUse);
	if(O_Lock == 1){
		generalListRemove(nodePtr, clientPtr->nodeLocked);
	}
	Pthread_mutex_unlock( &(clientPtr->lock) );
	
	errno = 0;
	return 0;
}

// toglie file da lista lock
// se non ottengono mutua esclusione termino / chiudo client e rifaccio struct
// return -1 fail 0 success setta errno
int clientUnlock(int clientId, TreeNode* nodePtr, ClientTable* tab){
	ClientInfo* clientPtr = clientGet(clientId, tab);
	if(clientPtr == NULL){
		// errno settato da clientGet
		return -1;
	}
	if( Pthread_mutex_lock( &(clientPtr->lock) ) != 0){
		errno = EPERM;
		return -1;
	}
	generalListRemove(nodePtr, clientPtr->nodeLocked);
	Pthread_mutex_unlock( &(clientPtr->lock) );
	
	errno = 0;
	return 0;
}

// dopo la rimozione di un file viene chiuso a tutti per evitare segFault
// se fallisce devo terminare il server
// da chiamare come ultima funzione di rimozione qunado nessuno ha puntatori a questo file
// return -1 fail 0 success setta errno
int clientFileDel(TreeNode* nodePtr, ClientTable* tab, ServerFile* filePtr){
	
	if( Pthread_mutex_lock( &(tab->lock) ) != 0){
		errno = EPERM;
		return -1;
	}
	// non chiamo lock su file (chiamata da chi vuolerimuovere il file)
	int* clientId = NULL;
	ClientInfo* clientPtr;
	while(clientId = generalListPop( filePtr->openList ), clientId != NULL){
		// non posso chiamare clientClose perche' perderei mutua esclusione
		clientPtr = clientGetNoLock(*clientId, tab);
		if(clientPtr != NULL){
			// se avanza tempo chiudere il client e non terminare completamente
			if( Pthread_mutex_lock( &(clientPtr->lock) ) != 0){
				Pthread_mutex_unlock( &(tab->lock) );
				errno = EPERM;
				return -1;
			}
			generalListRemove( (void*) nodePtr, clientPtr->nodeInUse );
			Pthread_mutex_unlock( &(clientPtr->lock) );
		}
		free(clientId);
	}
	if( filePtr->flagO_lock == 1){
		clientPtr = clientGetNoLock(  filePtr->lockOwner , tab);
		if( Pthread_mutex_lock( &(clientPtr->lock) ) != 0){
			Pthread_mutex_unlock( &(tab->lock) );
			errno = EPERM;
			return -1;
		}
		generalListRemove( (void*) nodePtr, clientPtr->nodeLocked );
		Pthread_mutex_unlock( &(clientPtr->lock) );
	}
	Pthread_mutex_unlock( &(tab->lock) );
	errno = 0;
	return 0;
}

// restituisce il posto nell'array in cui sta il client
int clientBucket(int clientId){
	return clientId % BASEDIM;
}


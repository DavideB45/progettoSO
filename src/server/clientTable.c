#include <clientTable.h>
#include <generalList.h> 
#include <files.h>
#include <utils.h>

#include <errno.h>
#include <stdlib.h>



int filePtrCompare( const void* file1, const void* file2){
	return *((ServerFile**) file1) == *((ServerFile**) file2);
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
			return 1;
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
		new_Client->fileLocked = newGeneralList(filePtrCompare, free);
		if(new_Client->fileLocked == NULL){
			pthread_mutex_destroy( &(new_Client->lock) );
			free(new_Client);
			Pthread_mutex_unlock( &(tab->lock ) );
			errno = ENOMEM;
			return -1;
		}
		new_Client->fileInUse = newGeneralList(filePtrCompare, free);
		if(new_Client->fileInUse == NULL){
			generalListDestroy(new_Client->fileLocked);
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
	return 1;
}

// mette file nella lista Open
// se non ottengo mutua esclusione non lo metto
int clientOpen(int client, ServerFile* filePtr, int O_Lock, ClientTable* tab);

// mette file nella lista lock
// se non ottengo mutua esclusione non lo metto
int clientLock(int client, ServerFile* filePtr, ClientTable* tab);



// chiude tutto quello che aveva aperto
// se non ottengo mutua esclusione lascio tutto aperto per chi verra' dopo
int destroyClient(int clientId, ClientTable *tab);

// toglie file dalla lista close
// se non ottengono la mutua esclusione lo lasciano aperto
// rischio segFault chiudo client (il prossimo avra' qualche file gia' aperto)
int clientClose(int client, ServerFile* filePtr, int O_Lock, ClientTable* tab);

// toglie file da lista lock
// se non ottengono mutua esclusione 
// rischio segFault chiudo client (il prossimo avra' qualche file gia' locked)
int clientUnlock(int client, ServerFile* filePtr, ClientTable* tab);

// dopo la rimozione di un file viene chiuso a tutti per evitare segFault
int clientFileDel(ServerFile* filePtr);

// restituisce il posto nell'array in cui sta il client
int clientBucket(int client);
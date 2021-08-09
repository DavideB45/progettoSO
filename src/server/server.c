#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// selfmade
#include <utils.h>
#include <server.h>
#include <FifoList.h>
#include <request.h>
#include <tree.h>

// connection
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>

#define SOCKET_FD srvGen.sockFD

// macro per usare l'array dei thread
#define PERSONAL_LOCK_ACQUIRE   Pthread_mutex_lock( &(srvGen.threadUse[threadId]->lock) )
#define PERSONAL_LOCK_RELEASE   Pthread_mutex_unlock( &(srvGen.threadUse[threadId]->lock) )
#define PERSONAL_FILE_SET(fPtr) srvGen.threadUse[threadId]->filePtr = fPtr
#define PERSONAL_LOCK_SIGNAL	pthread_cond_signal( &(srvGen.threadUse[threadId]->completedReq )
#define THREAD_LOCK_ACQUIRE(id)  Pthread_mutex_lock( &(srvGen.threadUse[id]->lock) )
#define THREAD_LOCK_RELEASE(id)  Pthread_mutex_unlock( &(srvGen.threadUse[id]->lock) )
#define THREAD_COND_WAIT(id)	pthread_cond_wait( &(srvGen.threadUse[id]->completedReq), &(srvGen.threadUse[id]->lock))
#define THREAD_FILE_GET(id)     srvGen.threadUse[id]->filePtr

#define PIPE_WRITE(client)  if( writen(srvGen.doneReq[1], &client, sizeof(int)) == -1){\
								perror("writen");\
								exit(EXIT_FAILURE);\
							}

ServerInfo srvGen;
FifoList resourceQueue;
FifoList logQueue;
TreeFile* fileStorage;

void readConfig(char* indirizzo);
int initServer(void);

void* dispatcher(void);
// ritorna il massimo FD da ascoltare
int updatemax(fd_set set, int maxFD);


// il puntatore a intero passato indica il suo posto nell'array
void* worker(void*);
// esegue una richiesta
void manageRequest(Request* req, int threadId);
// setta il flag in use di un file
// torna 1 se successo
// rorna 0 se fallisce
// non effettua controlli sugli argomenti passati
int tryUse(ServerFile* filePtr, Request* req, int closeOnFail);
// ritorna quando nessuno possiede un puntatore al file che voglio rimuovere
// da usare quando il file non e' piu' presente nell' albero
// ritorna 0 se completa n>0 altrimenti
int safeRemove(ServerFile* filePtr, int threadId);

// chiude la connessione con il client
int closeConnection(int client);
// apre un file (open = intero che definisce opzioni)
int openFile(Request* req, ServerFile* filePtr);
// scrive al client il contenuto del file
int readFile(Request* req, ServerFile* filePtr);
// scrive al client il contenuto di N files
int readNFiles(Request* req);
// scrive nel file richiesto dal client
int writeFile(Request* req, ServerFile* filePtr);
// appende al file richiesto dal client
int appendToFile(Request* req, ServerFile* filePtr);
// attiva la mutua esclusione su un file
int lockFileW(Request* req, ServerFile* filePtr);
// termina la mutua esclusione su un file
int unlockFileW(Request* req, ServerFile* filePtr);
// chiude il file per il client
int closeFile(Request* req, ServerFile* filePtr);
//rimuove il file dal server
int removeFile(Request* req, ServerFile* filePtr);


//////////////////////////////////////////////////////////////
// legge da un socket e lo chiude in caso di fallimento di readn
// informa il dispatcher
int readFormSocket(int sock, void* buff, int dim);
// invia al client errori che non chiudono la connessione
// in caso di fallimento di writen il socket viene chiuso
void sendClientError(int sock, int err);
// segnala al client un errore fatale 
// duarnte la lettura dal socket
void sendClientFatalError(int sock, int err);
//////////////////////////////////////////////////////////////

// reception

int main(int argc, char* argv[]){

	if(initServer() != 0){
		return 1;
	}
	


	

}

int initServer(void){
	
	readConfig("./servWork/file_config");

	// creo una pipe
	// [1] per scrivere [0] per leggere
	if(pipe(srvGen.doneReq) == -1){
		perror("pipe");
		exit(1);
	}

	unlink(srvGen.sockName);
	SOCKET(SOCKET_FD);

	//assegno un indirizzo al socket
	//struttura generica specializzata di volta in volta
	struct sockaddr_un sa;
	memset(&sa, 0, sizeof(sa));
	//inizializzo con i valori giusti
	strncpy(sa.sun_path, srvGen.sockName,UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;
				//cast neccessario		
	if( bind(SOCKET_FD,(struct sockaddr *) &sa, sizeof(sa) ) ){//creo socket
		perror("bind");
		exit(1);
	}
	//tiene un po' di persone in coda per non rifiuare 
	//se il server non e' subito pronto ad accettarle
	if(listen(SOCKET_FD,SOMAXCONN)){
		perror("listen");
		exit(1);
	}



	return 0;
}

void* dispatcher(void){
	
	int maxFD = SOCKET_FD > srvGen.doneReq[0] ? SOCKET_FD : srvGen.doneReq[0];
	int newConn;
	int resetConn;
	int* request;
	int nReady;
	fd_set set;
	fd_set rdSet;
	FD_ZERO(&set);
	FD_SET(SOCKET_FD, &set);

	while(srvGen.serverStatus == S_WORKING){
		rdSet = set;
		// se qualcosa non funziona con le interruzioni provare pselect
		nReady = select(maxFD + 1, &rdSet, NULL, NULL, NULL);
		if(nReady == -1){
			/* SELECT ERROR */
			if(errno != EINTR){
				exit(1);
			} else {
		/* mettere gestione della chiusura corretta in base allo stato del server dopo il segnale */
				continue;
			}
		}
		for(size_t fd = 0; fd <= maxFD; fd++){
			if(FD_ISSET(fd, &rdSet)){
				if(fd == SOCKET_FD){
					//ritorna un fd del scket che useremo per comunicare
					newConn = accept(SOCKET_FD ,NULL, 0);
					if(newConn == -1){
						/* ACCEPT ERROR */
						switch(errno){
						case EBADF:
							//fd non valido
							//continuo fino a che ho qualcuno da servire
							FD_CLR(SOCKET_FD, &set);
							if(maxFD == SOCKET_FD)
								maxFD = updatemax(set, maxFD);
				/* mettere il server in uno stato S_SLOW_CLOSE*/
						break;
						case EINTR:
							/* interrupt */
		/* mettere gestione della chiusura corretta in base allo stato del server dopo il segnale */
						break;
						case EMFILE:
							/* max file opened */
							continue;
						break;
						default:
							/* errori gravi */
							exit(1);
						break;
						}
					} else {
						if(newConn > 1023){
							/* fd troppo grande per poll */
							close(newConn);
						} else {
							/* METTO NUOVO CLIENT NELLA MASCHERA */
							if(maxFD < newConn)
								maxFD = newConn;
							FD_SET(newConn, &set);
						}
					}
				} else {
					if(fd == srvGen.doneReq[0]){
						/* RESET FROM PIPE */
						switch( readn(fd, (void*) &resetConn, sizeof(int)) ){
							case -1:
								// errore generico
								// la pipe e' rovinata
								exit(1);
							break;
							case  0:
								// pipe chiusa (non la guardo piu')
								if(close(srvGen.doneReq[0]) != 0){
									perror("close pipe (select)");
								}
								FD_CLR(fd, &set);
							break;
							case 1:
								if(IS_TO_RESET(resetConn)){
									//torno ad ascoltare su quel socket
									if(maxFD < GET_FD(resetConn))
										maxFD = GET_FD(resetConn);
									FD_SET(GET_FD(resetConn), &set);
								} else {
									// chiudo il socket
									FD_CLR(GET_FD(resetConn), &set);
									if(close(GET_FD(resetConn))){
										perror("close client");
									}									
								}
							break;
						}
					} else {
						/* RICHIESTA DA CLIENT */
						FD_CLR(fd, &set);
						if(maxFD == fd)
							maxFD = updatemax(set, maxFD);
						request = malloc(sizeof(int));
						if(request == NULL){
							perror("no mem");
							exit(1);
						}
						*request = fd;
						if( insert(srvGen.toServe, (void*) request) == 0){
							printf("Fifo error\n");
							exit(1);
						}
					}

				}
			
			}
			
		}
		
	}	
	
}

void readConfig(char* indirizzo){
	
	srvGen.maxFileNum = 10;
	srvGen.maxFileDim = 30;
	srvGen.n_worker    = 3;

	FILE * filePtr = NULL;
	filePtr = fopen(indirizzo, "r");
	if(filePtr == NULL){
		printf("fileNotFound\n");
		/////////////////////////////////inserire nomi e thread
		return;
	}
	int num;
	char str[30];

	fscanf(filePtr, "%*[^_]");

	if(fscanf(filePtr, "%*[_n_worker]%*[ :=\t]%d%*[ \n]", &num) == 1){
		if(num > 0){
			srvGen.n_worker = num;		
		}
	}
	srvGen.threadUse = malloc(sizeof(ThreadInfo*)*srvGen.n_worker);
	if(srvGen.threadUse == NULL){
		perror("malloc threadUse[]");
		exit(EXIT_FAILURE);
	}
	for(size_t i = 0; i < srvGen.n_worker; i++){
		srvGen.threadUse[i] = malloc(sizeof(ThreadInfo));
		if(srvGen.threadUse[i] == NULL){
			perror("malloc threadUse");
			exit(EXIT_FAILURE);
		}
		if( Pthread_mutex_init( &(srvGen.threadUse[i]->lock) ) != 0){
			perror("mutex init threadUse");
			exit(EXIT_FAILURE);
		}
		if(pthread_cond_init(&(srvGen.threadUse[i]->completedReq), NULL) != 0){
			perror("cond init threadUse");
			exit(EXIT_FAILURE);
		}
		srvGen.threadUse[i]->filePtr = NULL;
	}
	printf("worker : %d\n", srvGen.n_worker);

	if(fscanf(filePtr, "%*[_max_file]%*[ :=\t]%d\n", &num) == 1){
		if(num > 0){
			srvGen.maxFileNum = num;
		}
	}
	printf("maxFil : %d\n", srvGen.maxFileNum);

	if(fscanf(filePtr, "%*[_max_dim]%*[ :=\t]%d%*[\n MbBm]", &num) == 1){
		if(num > 0){
			srvGen.maxFileDim = num;
		}
	}
	printf("maxDim : %d\n", srvGen.maxFileDim);

	if(fscanf(filePtr, "%*[_socket_name]%*[ :=\t]%s\n", str) != 1){
		str[0] = '0';
		str[1] = 0;
	}
	if( (strlen(str) == 1 && str[0] == '0') || (strcmp(str, "_file_log_name") == 0) ){
		srvGen.sockName = malloc(18);
		if(srvGen.sockName == NULL)
			exit(1);
		strcpy(srvGen.sockName, "./servWork/socket");
	} else {
		srvGen.sockName = malloc( strlen(str) + 1 + 11 );
		if(srvGen.sockName == NULL)
			exit(1);
		strcpy(srvGen.sockName, "./servWork/");
		strcat(srvGen.sockName, str);
	}
	printf("nameSo : %s\n", srvGen.sockName);

	if(fscanf(filePtr, "%*[_file_log_name]%*[ :=\t]%s\n", str) != 1){
		str[0] = '0';
		str[1] = 0;
	}
	if(strlen(str) == 1 && str[0] == '0'){
		srvGen.logName = malloc(15);
		if(srvGen.logName == NULL)
			exit(1);
		strcpy(srvGen.logName, "./servWork/log");
	} else {
		srvGen.logName = malloc( strlen(str) + 1 + 11);
		if(srvGen.logName == NULL)
			exit(1);
		strcpy(srvGen.logName, "./servWork/");
		strcat(srvGen.logName, str);	
	}
	printf("nameLo : %s\n", srvGen.logName);
}

int updatemax(fd_set set, int maxFD){
	for(int i = (maxFD - 1); i>=0 ; --i ){
		if(FD_ISSET(i, &set)) 
			return i;
	}
	return -1;
}

void* worker(void* idThread){
	int threadId = *((int*) idThread);
	int* readSocK;
	readSocK = (int*) pop(&(srvGen.toServe));
	if(readSocK == NULL){
		pthread_exit(NULL);
	} else {
		int clId = *readSocK;
		int oper = 0;
		switch( readn(clId, &oper, sizeof(int)) ){
		case -1:
			/* errore in lettura non risolvibile */			
			sendClientFatalError(clId, IMPOSSIBLE_READ);
		break;
		case 0:
			/* il client ha chiuso il socket */
			CONN_MARK(clId, NOT_CONNECTED);
			PIPE_WRITE(clId)
		break;
		default:
			Request* req = newRequest(oper, clId, NULL, 0, NULL);
			manageRequest(req, threadId);
		break;
		}
	} 
	
}

void manageRequest(Request* req, int threadId){
	ServerFile* filePtr = NULL;
	Request* newReq = NULL;
	enum operResult result = NONE;
	int buffInt = 0;
	do{
		switch(GET_OP(req->oper)){
		case CLOSE_CONNECTION:
			/* non dovrebbe arrivare qui' a seconda dell'implementazione */
			closeConnection(req->client);
			printf("CLOSE_CONNECTION\n");
		break;
		case OPEN_FILE:
			printf("OPEN_FILE\n");
			if(req->sFileName == NULL){
				/* e' la prima volta che vediamo questa richiesta */
				int dim = GET_PATH_DIM(req->oper);
				req->sFileName = malloc(dim + 1);
				if(req->sFileName == NULL){
					/* non c'e' spazio */
					sendClientError(req->client, NO_MEMORY);
					result = FAILED_STOP;
				} else {
					/* c'e' spazio */
					if( readFormSocket(req->client, req->sFileName, dim + 1) == 1){
						/* tutto ok */
						if( PERSONAL_LOCK_ACQUIRE != 0){
							/* non posso eseguire */
							perror("Personal lock acquire");
							sendClientError(req->client, UNKNOWN_ERROR);
							result = FAILED_STOP;
						} else {
							filePtr = TreeFileFind(fileStorage, req->sFileName);
							if(filePtr == NULL){
								PERSONAL_LOCK_RELEASE;
								/* file non trovato */
								if(errno == 0){
									sendClientError(req->client, NO_SUCH_FILE);
								} else{
									sendClientError(req->client, UNKNOWN_ERROR);
								}
								result = FAILED_STOP;
							} else {
								/* file trovato (eseguo la richiesta) */
								PERSONAL_FILE_SET(filePtr);
								PERSONAL_LOCK_RELEASE;
								if(tryUse(filePtr, req, FALSE)){
									result = openFile(req, filePtr);
								} else {
									result = FAILED_STOP;
								}
							}
						}
					} else {
						/* non ho letto il nome */
						sendClientFatalError(req->client, IMPOSSIBLE_READ);
						result = FAILED_STOP;
					}
				}
				/*  alla fine di questo if filePtr non puo' essere NULL
					nel caso lo sia non effettueremo altri cicli perche'
					non abbiamo un file su cui operare
					quindi se siamo nel ramo else signifia che quell'operazione
					era stata gia' trovata da un altro client in precedenza
					quindi e' stata rimossa dalla lista di richieste del file */
			} else {
				result = openFile(req, filePtr);
			}
			
		break;
		case READ_FILE:
			printf("READ_FILE\n");
			readFile(req, filePtr);
		break;
		case READ_N_FILES:
			printf("READ_N_FILES\n");
			readNFiles(req);
		break;
		case WRITE_FILE:
			printf("WRITE_FILE\n");
			writeFile(req, filePtr);
		break;
		case APPEND_TO_FILE:
			printf("APPEND_TO_FILE\n");
			appendToFile(req, filePtr);
		break;
		case LOCK_FILE:
			printf("LOCK_FILE\n");
			lockFileW(req, filePtr);
		break;
		case UNLOCK_FILE:
			printf("UNLOCK_FILE\n");
			unlockFileW(req, filePtr);
		break;
		case CLOSE_FILE:
			printf("CLOSE_FILE\n");
			closeFile(req, filePtr);
		break;
		case REMOVE_FILE:
			printf("REMOVE_FILE\n");
			removeFile(req, filePtr);
		break;
		default:
			printf("operazione sconosciuta\n");
		break;
		}

		if( filePtr != NULL && (result == FAILED_CONT || result == COMPLETED_CONT) ){
			/* devo continuare a lavorare */
			if( Pthread_mutex_lock( &(filePtr->lock) ) == -1){
				perror("lockFile");
				TreeFileRemove(fileStorage, req->sFileName);
				//////////informa file di log////////////////////////////////////////////
				result = FILE_DELETED;
			} else {
				newReq = generalListPop(filePtr->requestList);
				if(newReq == NULL){
					/* ho finito le richieste residue */
					filePtr->flagUse = 0;
					if(result == FAILED_CONT){
						result = FAILED_STOP;
					} else {
						result = COMPLETED_STOP;
					}
				}
				Pthread_mutex_unlock( &(filePtr->lock) );
				destroyRequest(&req);
				req = newReq;
			}
		}

	}while( req != NULL && result != FAILED_CONT && result != COMPLETED_CONT );
	
	if(result == FILE_DELETED){
		if(safeRemove(filePtr, threadId) != 0){
			if( GET_OP(req->oper) == REMOVE_FILE){
				sendClientError(req->client, UNKNOWN_ERROR);
				destroyRequest(&req);
			} else {
				// devo rimuoverlo a causa di un errore
				// termino?
			}
		} else{
			/* mi occupo di informare gli altri client*/
			while(req = generalListPop(filePtr->requestList), req != NULL){
				/* code */
			}
			
			destroyServerFile(filePtr);
		}
	}
	/* gestire gli altri casi */
	
	/* mettere NULL come file che sto gestendo */
}


int readFormSocket(int sock, void* buff, int dim){
	int buffInt;
	switch( readn(sock, buff, dim) ){
	case -1:
		/* impossible read */
		buffInt = IMPOSSIBLE_READ;
		writen(sock, &buffInt, sizeof(int));
		buffInt = sock;
		CONN_MARK(buffInt, NOT_CONNECTED);
		PIPE_WRITE(buffInt);
		return 0;
	break;
	case 0:
		/* client chiuso EOF */
		buffInt = sock;
		CONN_MARK(buffInt, NOT_CONNECTED);
		PIPE_WRITE(buffInt);
		return 0;
	break;
	default:
		return 1;
	break;
	}
}

void sendClientError(int sock, int err){
	int buffInt;
	if( writen(sock, &err, sizeof(int)) == 1){
		/* riesco a informare il client */
		buffInt = sock;
		CONN_MARK(buffInt, CONNECTED);
		PIPE_WRITE(buffInt);
	} else {
		/* non riesco a informare il client */
		buffInt = sock;
		CONN_MARK(buffInt, NOT_CONNECTED);
		PIPE_WRITE(buffInt);
	}
}

void sendClientFatalError(int sock, int err){
	int buffInt = err;
	writen(sock, &buffInt, sizeof(int));
	buffInt = sock;
	CONN_MARK(buffInt, NOT_CONNECTED);
	PIPE_WRITE(buffInt);
}

int tryUse(ServerFile* filePtr, Request* req, int closeOnFail){
	
	if( Pthread_mutex_lock( &(filePtr->lock)) != 0){
		if(closeOnFail){
			sendClientFatalError(req->client ,UNKNOWN_ERROR_F);
		} else {
			sendClientError(req->client, UNKNOWN_ERROR);
		}
		return 0;
	}
	if(filePtr->flagUse){
		if( generalListInsert(filePtr->requestList, req) == 0){
			/* operazione fallita (non ho messo in coda) */
			Pthread_mutex_unlock(&(filePtr->lock));
			if(closeOnFail){
				sendClientFatalError(req->client ,UNKNOWN_ERROR_F);
			} else {
				sendClientError(req->client, UNKNOWN_ERROR);
			}
		} else {
			/* richiesta messa in coda */
			Pthread_mutex_unlock(&(filePtr->lock));
		}
		return 0;
	} else {
		/* il thread inizia a gestire il file */
		filePtr->flagUse = 1;
		Pthread_mutex_unlock(&(filePtr->lock));
		return 1;
	}
	
}

int safeRemove(ServerFile* filePtr, int threadId){
	for(size_t i = 0; i < srvGen.n_worker; i++){
		if(threadId != i){
			if(THREAD_LOCK_ACQUIRE(i) != 0){
				return i + 1;
			}
			while(filePtr == THREAD_FILE_GET(i)){
				if(THREAD_COND_WAIT(i) != 0){
					THREAD_LOCK_RELEASE(i);
					return i + 1;
				}
			}
			THREAD_LOCK_RELEASE(i);
		}	
	}
	return 0;
}


int closeConnection(int client);
int openFile(Request* req, ServerFile* filePtr);
int readFile(Request* req, ServerFile* filePtr);
int readNFiles(Request* req);
int writeFile(Request* req, ServerFile* filePtr);
int appendToFile(Request* req, ServerFile* filePtr);
int lockFileW(Request* req, ServerFile* filePtr);
int unlockFileW(Request* req, ServerFile* filePtr);
int closeFile(Request* req, ServerFile* filePtr);
int removeFile(Request* req, ServerFile* filePtr){

}

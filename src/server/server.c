#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

// selfmade
#include <utils.h>
#include <server.h>
#include <FifoList.h>
#include <request.h>
#include <tree.h>
#include <clientTable.h>
#include <logFun.h>

// connection
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>

#define SOCKET_FD srvGen.sockFD

#define PIPE_WRITE(client)  if( writen(srvGen.doneReq[1], &client, sizeof(int)) == -1){\
								perror("writenPipe");\
								exit(EXIT_FAILURE);\
							}

#define DISCONN_CLIENT(_client_) disconnectClient(_client_, resourceTable);\
								 CONN_MARK(_client_, NOT_CONNECTED);\
								 PIPE_WRITE(_client_);
								 
#define LISTEN_CLIENT(_client_) CONN_MARK(_client_, CONNECTED);\
								PIPE_WRITE(_client_);

#define LOG_INSERT(logInfox) if( logInfox != NULL){\
								if(insert(logQueue, logInfox) == 0){\
									destroyLogOp(logInfox);\
								}\
							 }

ServerInfo srvGen;
ClientTable* resourceTable;
FifoList* logQueue;
TreeFile* fileStorage;
volatile __sig_atomic_t serverStatus;

void readConfig(char* indirizzo);
int initServer(void);
int createThreads(void);
void collectThreads(void);

int setNewHand(void);
void exitFun(void);
void sigIntQuit(int);
void sigHup(int);

void dispatcher(void);
// ritorna il massimo FD da ascoltare
int updatemax(fd_set set, int maxFD);

void* logThread(void*);

// il puntatore a intero passato indica il suo posto nell'array
void* worker(void*);
// esegue una richiesta
void manageRequest(Request* req, int threadId);
// ret -1 errore
// ret 0 delayed
// ret 1 utilizzabile
// non effettua controlli sugli argomenti passati
int tryUse(TreeNode* nodePtr, Request* req, int closeOnFail);
// ritorna 1 se il file puo' essere usato da un client
// ritorna 0 se il file e' lockato da un altro client 
int fileUsePermitted(int client, ServerFile* filePtr);

// chiude la connessione con il client
int closeConnectionW(int client);
// apre un file (open = intero che definisce opzioni)
int openFileW(Request* req, TreeNode** nodePtrP);
// scrive al client il contenuto del file
int readFileW(Request* req, TreeNode *Ptr, int threadId);
// scrive al client il contenuto di N files
int readNFilesW(Request* req, int threadId);
// scrive nel file richiesto dal client
int writeFileW(Request* req, TreeNode* nodePtr, int threadId);
// appende al file richiesto dal client
int appendToFileW(Request* req, TreeNode* nodePtr,int threadId, int logOpKind);
// attiva la mutua esclusione su un file
int lockFileW(Request* req, TreeNode* nodePtr, int threadId);
// termina la mutua esclusione su un file
int unlockFileW(Request* req, TreeNode* nodePtr, int threadId);
// chiude il file per il client
int closeFileW(Request* req, TreeNode* nodePtr, int threadId);
//rimuove il file dal server
int removeFileW(Request* req, TreeNode* nodePtr, int threadId);

// dopo la rimozione di un file pulisce la lista di richieste ancora in coda
void informClientDelete(ServerFile* filePtr);


//////////////////////////////////////////////////////////////
// legge il nome di un file da un socket
// se ci riesce lo cerca nell'albero
// ritorna il puntatore al file
// se va a buon fine *result == NONE
TreeNode* getFileFromSocket(Request* req, enum operResult* result, int threadId);
// legge da un socket e lo chiude in caso di fallimento di readn
// informa il dispatcher
// ritorna -1 fallimento
// ritorna 0 altrimenti
int readFormSocket(int sock, void* buff, int dim);
// invia al client il risultato dell' operazione
// in caso di successo ritorna 0
// in caso di fallimento chiude il socket e ritorna -1
// dim include '\0' se presente
int sendClientResult(int client, void* reply, int dim);
// invia al client errori che non chiudono la connessione
// in caso di fallimento di writen il socket viene chiuso
void sendClientError(int sock, int err);
// segnala al client un errore fatale 
// duarnte la lettura dal socket
void sendClientFatalError(int sock, int err);
//////////////////////////////////////////////////////////////


int main(int argc, char* argv[]){

	// bloccare i segnali
	sigset_t newSet;
	sigemptyset(&newSet);
	sigaddset(&newSet, SIGINT);
	sigaddset(&newSet, SIGPIPE);
	sigaddset(&newSet, SIGHUP);
	sigaddset(&newSet, SIGQUIT);
	sigset_t oldSet;

	pthread_sigmask(SIG_SETMASK, &newSet, &oldSet);
	

	// controlla di non aver lasciato nulla
	if(initServer() != 0){
		destroyTreeFile(fileStorage);
		destroyClientTable(resourceTable);
		destroyList(logQueue, destroyLogOp);
		exit(EXIT_FAILURE);
	}

	if(createThreads() != 0){
		exit(1);
	}

	if(setNewHand() != 0){
		exit(EXIT_FAILURE);
	}

	if(atexit(exitFun)){
		return -1;
	}

	sigaddset(&oldSet, SIGPIPE);
	pthread_sigmask(SIG_SETMASK, &oldSet, NULL);
	
	dispatcher();

	collectThreads();
}

int initServer(void){


	fileStorage = newTreeFile();
	resourceTable = newClientTable();
	logQueue = newList();

	if(fileStorage == NULL || resourceTable == NULL || logQueue == NULL){
		return -1;
	}
	

	// leggere argv

	readConfig("./servWork/file_config");

	// [1] per scrivere [0] per leggere
	if(pipe(srvGen.doneReq) == -1){
		perror("pipe");
		exit(1);
	}
	srvGen.toServe = newList();
	if(srvGen.toServe == NULL){
		exit(EXIT_FAILURE);
	}
	srvGen.clientNum = 0;
	srvGen.clientMax = 0;
	serverStatus = S_WORKING;

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

int createThreads(void){
	int* id;
	if( pthread_create( &(srvGen.threadLog), NULL, logThread,  NULL) != 0){
			perror("pthread create");
			return -1;
	}
	for(size_t i = 0; i < srvGen.n_worker; i++){
		if(id = malloc(sizeof(int)), id == NULL){
			perror("malloc Int");
			return -1;
		}
		*id = i; 
		if( pthread_create( &(srvGen.threadUse[i]->thread), NULL, worker,  id) != 0){
			perror("pthread create");
			return -1;
		}
	}
	return 0;
}

void collectThreads(void){
	int nullSended = 0;
	for(size_t i = 0; i < srvGen.n_worker; i++){
		nullSended += insert(srvGen.toServe, NULL);
	}
	int* threadRet;
	if(nullSended >= srvGen.n_worker){
		for(int i = 0; i < srvGen.n_worker; i++){
			pthread_join(srvGen.threadUse[i]->thread,(void**) &threadRet);
			free(threadRet);
			free(srvGen.threadUse[i]);
		}
	}
	LogOp* endLog = newLogOp(0, NULL, SERVER, 0, 0, 0, 0);
	if(insert(logQueue, endLog) == 0){
		destroyLogOp(endLog);
	} else {
		pthread_join(srvGen.threadLog, NULL);
	}
}

int setNewHand(void){
	struct sigaction newSigint;
	memset( &newSigint, 0, sizeof(newSigint) );
	newSigint.sa_handler = sigIntQuit;
	sigaddset( &(newSigint.sa_mask), SIGQUIT );
	if(sigaction(SIGINT, &newSigint, NULL)){
		perror("sigaction 1");
		return -1;
	}

	struct sigaction newSigquit;
	memset( &newSigquit, 0, sizeof(newSigquit) );
	newSigquit.sa_handler = sigIntQuit;
	sigaddset( &(newSigquit.sa_mask), SIGINT );
	if(sigaction(SIGQUIT, &newSigquit, NULL)){
		perror("sigaction 1");
		return -1;
	}

	struct sigaction newSigHup;
	memset( &newSigHup, 0, sizeof(newSigHup) );
	newSigHup.sa_handler = sigIntQuit;
	if(sigaction(SIGHUP, &newSigHup, NULL)){
		perror("sigaction 1");
		return -1;
	}

	return 0;
}

void exitFun(void){
	
	
	
	destroyClientTable(resourceTable);
	destroyTreeFile(fileStorage);
	destroyList(logQueue, destroyLogOp);

	destroyList(srvGen.toServe, free);
	free(srvGen.threadUse);
	free(srvGen.logName);
	free(srvGen.sockName);
	close(srvGen.sockFD);
	close(srvGen.doneReq[1]);
	close(srvGen.doneReq[0]);
	
}

void sigIntQuit(int signal){
	serverStatus = S_FAST_CLOSE;
}

void sigHup(int signal){
	serverStatus = S_SLOW_CLOSE;
}

void dispatcher(void){
	
	int maxFD = SOCKET_FD > srvGen.doneReq[0] ? SOCKET_FD : srvGen.doneReq[0];
	int newConn;
	int resetConn;
	int* request;
	int nReady;
	fd_set set;
	fd_set rdSet;
	FD_ZERO(&set);
	FD_SET(SOCKET_FD, &set);
	FD_SET(srvGen.doneReq[0], &set);
	

	while(serverStatus == S_WORKING || (serverStatus == S_SLOW_CLOSE && srvGen.clientNum > 0) ){
		if(serverStatus == S_SLOW_CLOSE){
			FD_CLR(SOCKET_FD, &set);
			if(maxFD == SOCKET_FD)
				maxFD = updatemax(set, maxFD);
		}
		rdSet = set;
		// se qualcosa non funziona con le interruzioni provare pselect
		
		nReady = select(maxFD + 1, &rdSet, NULL, NULL, NULL);
		if(nReady == -1){
			/* SELECT ERROR */
			if(errno != EINTR){
				serverStatus = S_FAST_CLOSE;
			}
		} else {
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
							serverStatus = S_SLOW_CLOSE;
							FD_CLR(SOCKET_FD, &set);
							if(maxFD == SOCKET_FD)
								maxFD = updatemax(set, maxFD);
						break;
						case EINTR:
							/* interrupt */
							fd = maxFD + 10;
						break;
						case EMFILE:
							/* max file opened */
							continue;
						break;
						default:
							serverStatus = S_SLOW_CLOSE;
							FD_CLR(SOCKET_FD, &set);
							if(maxFD == SOCKET_FD)
								maxFD = updatemax(set, maxFD);
						break;
						}
					} else {
						if(newConn > 1023){
							/* fd troppo grande per select */
							int ret = SERVER_FULL;
							writen(newConn, &ret, sizeof(int));
							close(newConn);
						} else {
							/* METTO NUOVO CLIENT NELLA MASCHERA */
							printf("NUOVA CONNESSIONE %d\n", newConn);
							LogOp* infoLog = newLogOp(OPEN_CONNECTION, NULL, newConn, SERVER,1, 0, 0);
							LOG_INSERT(infoLog);
							(srvGen.clientNum)++;
							if(srvGen.clientNum > srvGen.clientMax){
								srvGen.clientMax = srvGen.clientNum;
							}
							
							newClient(newConn, resourceTable);///////////vedere se spostare e farlo fare al client
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
								serverStatus = S_FAST_CLOSE;
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
									(srvGen.clientNum)--;
									LogOp* infoLog = newLogOp(CLOSE_CONNECTION, NULL, GET_FD(resetConn), SERVER, 1, 0, sizeof(int));
									LOG_INSERT(infoLog);
									FD_CLR(GET_FD(resetConn), &set);
									if(close(GET_FD(resetConn))){
										perror("close client");
									}									
								}
							break;
						}
					} else {
						/* RICHIESTA DA CLIENT */
						request = malloc(sizeof(int));
						if(request == NULL){
							perror("no mem");
							break;
						}
						FD_CLR(fd, &set);
						if(maxFD == fd)
							maxFD = updatemax(set, maxFD);
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
	return;
}

void readConfig(char* indirizzo){
	

	
	fileStorage->maxFileDim = 30;
	fileStorage->maxFileNum = 10;
	srvGen.n_worker    = 3;

	FILE * filePtr = NULL;
	filePtr = fopen(indirizzo, "r");
	if(filePtr == NULL){
		printf("fileNotFound\n");
		
		srvGen.sockName = malloc(18);
		if(srvGen.sockName == NULL)
			exit(1);
		strcpy(srvGen.sockName, "./servWork/socket");

		srvGen.logName = malloc(15);
		if(srvGen.logName == NULL)
			exit(1);
		strcpy(srvGen.logName, "./servWork/log");
		
		return;
	} else {
		int num;
		char str[30];

		fscanf(filePtr, "%*[^_]");

		if(fscanf(filePtr, "%*[_n_worker]%*[ :=\t]%d%*[ \n]", &num) == 1){
			if(num > 0){
				srvGen.n_worker = num;		
			}
		}

		if(fscanf(filePtr, "%*[_max_file]%*[ :=\t]%d\n", &num) == 1){
			if(num > 0){
				fileStorage->maxFileNum = num;
			}
		}

		if(fscanf(filePtr, "%*[_max_dim]%*[ :=\t]%d%*[\n MbBm]", &num) == 1){
			if(num > 0){
				fileStorage->maxFileDim = num;
			}
		}
		
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
	}
	printf("worker : %d\n", srvGen.n_worker);

	printf("maxFil : %d\n", fileStorage->maxFileNum);
	
	printf("maxDim : %d\n", fileStorage->maxFileDim);

	printf("nameSo : %s\n", srvGen.sockName);

	printf("nameLo : %s\n", srvGen.logName);
	fclose(filePtr);
}

int updatemax(fd_set set, int maxFD){
	for(int i = (maxFD - 1); i>=0 ; --i ){
		if(FD_ISSET(i, &set)) 
			return i;
	}
	return -1;
}

void* logThread(void* arg){
	FILE * filePtr = NULL;
	filePtr = fopen(srvGen.logName, "a+");
	if(filePtr == NULL){
		printf("fileNotFound\n");
		return NULL;
	}
	time_t approxTime = time(NULL);
	fprintf(filePtr, "\nAPERTURA SERVER %s", ctime(&approxTime));
	LogOp* toWrite = pop(logQueue);
	while(toWrite != NULL){
		fprintf(filePtr, "%3d : %3d : ", toWrite->client, toWrite->thread);
		if(CLIENT_OP(toWrite)){
			fprintf(filePtr, "%2d %17s | ", toWrite->opType, operatToString(toWrite->opType, FALSE));
			fprintf(filePtr, "res = %d | ", toWrite->result);
			if(toWrite->opType == OPEN_FILE){
				if(WITH_CREA(toWrite)){
					fprintf(filePtr, "created | ");
				}
				if(WITH_LOCK(toWrite)){
					fprintf(filePtr, "locked | ");
				}
			} else {
				if(toWrite->result == 1){
					if(toWrite->deltaDim != 0){
						fprintf(filePtr, "editDim = %+d | ", toWrite->deltaDim);
					}
				}
			}
			if(toWrite->fileName != NULL){
				fprintf(filePtr, "%s | ", toWrite->fileName);
			}
			fprintf(filePtr, "ret dim = %d |", toWrite->dimReturn);
		}
		if(LRU_REPLACE(toWrite)){
			if(FILE_REMOVED(toWrite) == 1 && toWrite->fileName != NULL){
				fprintf(filePtr, "file rem = %s freed mem = %d  | ", toWrite->fileName, FREED_MEMORY(toWrite));
			} else {
				fprintf(filePtr, "file rem = %d freed mem = %d | ", FILE_REMOVED(toWrite), FREED_MEMORY(toWrite));
			}
		}
		if(LRU_NEW_MAX(toWrite)){
			switch(NEW_MAX_TIPE(toWrite)){
			case MAXDIM:
				fprintf(filePtr, "MAX dim = %d curr n file = %d | ", NEW_DIM(toWrite), NEW_FILE_N(toWrite));
			break;
			case MAXFILE:
				fprintf(filePtr, "curr dim = %d MAX n file = %d | ", NEW_DIM(toWrite), NEW_FILE_N(toWrite));
			break;
			case MAXALL:
				fprintf(filePtr, "MAX dim = %d MAX n file = %d | ", NEW_DIM(toWrite), NEW_FILE_N(toWrite));
			break;
			}
		}
		if(SERV_CLOSE(toWrite)){
			fprintf(filePtr, "%d : N file = %d DIM tot = %d | ",toWrite->client, fileStorage->fileCount, fileStorage->filedim);
		}
		
		
		fprintf(filePtr, "%s", ctime( &(toWrite->execTime)) );
		fflush(filePtr);
		if(!SERV_CLOSE(toWrite)){
			destroyLogOp(toWrite);
			toWrite = pop(logQueue);
		} else {
			destroyLogOp(toWrite);
			toWrite = NULL;
		}
	}
	TreeNode* nodeCurr = fileStorage->mostRecentLRU;
	while(nodeCurr != NULL){
		if(nodeCurr->sFile != NULL){
			fprintf(filePtr, "%s\n", nodeCurr->name);
			nodeCurr = nodeCurr->lessRecentLRU;
		}
	}
	printf("non scrivo nel file piccione\n");
	fclose(filePtr);
	return NULL;
}


void* worker(void* idThread){
	
	int threadId = *((int*) idThread);
	free(idThread);
printf("ciao, sono nato %d\n", threadId);
	int* readSocK;
	while (1){
	
	readSocK = (int*) pop( srvGen.toServe );
	if(readSocK == NULL){
		pthread_exit(NULL);
	} else {
		int clId = *readSocK;
		int oper = 0;
		free(readSocK);
		switch( readn(clId, &oper, sizeof(int)) ){
		case -1:
			/* errore in lettura non risolvibile */		
			sendClientFatalError(clId, IMPOSSIBLE_READ);
		break;
		case 0:
			/* il client ha chiuso il socket */
			DISCONN_CLIENT(clId);
		break;
		default:;
			printf("%d : richiesta da %d\n",threadId, clId);
			Request* req = newRequest(oper, clId, NULL, 0, NULL);
			manageRequest(req, threadId);
		break;
		}
	}
	}
	pthread_exit(NULL);
	// return NULL;
}

void manageRequest(Request* req, int threadId){
	TreeNode* nodePtr = NULL;
	Request* newReq = NULL;
	LogOp* infoLog = NULL;
	enum operResult result = NONE;
	do{
		switch(GET_OP(req->oper)){
		case CLOSE_CONNECTION:
			/* non dovrebbe arrivare qui' */
			DISCONN_CLIENT(req->client);
			printf("CLOSE_CONNECTION\n");
		break;
		case OPEN_FILE:
			printf("OPEN_FILE\n");
			if(req->sFileName == NULL){
				/* e' la prima volta che vediamo questa richiesta */
				nodePtr = getFileFromSocket(req, &result, threadId);
				if(result == NONE){
					if(nodePtr == NULL){
						/* file non trovato forse lo creo*/
						result = openFileW(req, &nodePtr);
					} else {
						/* file trovato (eseguo la richiesta) */
						// da sistemare per delayed
						switch(tryUse(nodePtr, req, FALSE)){
						case -1:
							result = FAILED_STOP;
						break;
						case 0:
							result = DELAYED;
						break;
						case 1:
							result = openFileW(req, &nodePtr);
						break;
						}
					}
				}
				/*  alla fine di questo if nodePtr non puo' essere NULL
					nel caso lo sia non effettueremo altri cicli perche'
					non abbiamo un file su cui operare
					quindi se siamo nel ramo else signifia che quell'operazione
					era stata gia' trovata da un altro client in precedenza
					quindi e' stata rimossa dalla lista di richieste del file */
			} else {
				result = openFileW(req, &nodePtr);
			}
			int flag = 0;
			if(GET_O_CREATE(req->oper)){
				flag = CREA_OPEN;
			}
			if(GET_O_LOCK(req->oper)){
				flag = flag | LOCK_OPEN;
			}
			if(result == COMPLETED_CONT){
				infoLog = newLogOp(OPEN_FILE, req->sFileName, req->client, threadId, 1, flag , sizeof(int));
				LOG_INSERT(infoLog);
			} else {
				if(result == COMPLETED_STOP){
					infoLog = newLogOp(OPEN_FILE, req->sFileName, req->client, threadId, 1, flag , sizeof(int));
					LOG_INSERT(infoLog);
				} else {
					if(result != DELAYED){
						infoLog = newLogOp(OPEN_FILE, req->sFileName, req->client, threadId, 0, flag, sizeof(int));
						LOG_INSERT(infoLog);
					}
				}
			}
		break;
		case READ_FILE:
			printf("READ_FILE\n");
			if(req->sFileName == NULL){
				nodePtr = getFileFromSocket(req, &result, threadId);
				if(result == NONE){
					if(nodePtr == NULL){
						sendClientFatalError(req->client, NO_SUCH_FILE_F);
						result = FAILED_STOP;
					} else {
						/* file trovato (eseguo la richiesta) */
						switch(tryUse(nodePtr, req, FALSE)){
						case -1:
							result = FAILED_STOP;
						break;
						case 0:
							result = DELAYED;
						break;
						case 1:
							result = readFileW(req, nodePtr, threadId);
						break;
						}
					}
				}
			} else {
				result = readFileW(req, nodePtr, threadId);
			}
		break;
		case READ_N_FILES:
			printf("READ_N_FILES\n");
			readNFilesW(req, threadId);
		break;
		case WRITE_FILE:
			printf("WRITE_FILE\n");
			if(req->sFileName == NULL){
				nodePtr = getFileFromSocket(req, &result, threadId);
				if(result == NONE){
					if(nodePtr == NULL){
						sendClientFatalError(req->client, NO_SUCH_FILE_F << 24);
						result = FAILED_STOP;
					} else {
						/* file trovato (eseguo la richiesta) */
						switch(tryUse(nodePtr, req, FALSE)){
						case -1:
							result = FAILED_STOP;
						break;
						case 0:
							result = DELAYED;
						break;
						case 1:
							result = writeFileW(req, nodePtr, threadId);
						break;
						}
					}
				}
			} else {
				result = writeFileW(req, nodePtr, threadId);
			}
		break;
		case APPEND_TO_FILE:
			printf("APPEND_TO_FILE\n");
			if(req->sFileName == NULL){
				nodePtr = getFileFromSocket(req, &result, threadId);
				if(result == NONE){
					if(nodePtr == NULL){
						sendClientFatalError(req->client, NO_SUCH_FILE_F << 24);
						result = FAILED_STOP;
					} else {
						/* file trovato (eseguo la richiesta) */
						switch(tryUse(nodePtr, req, TRUE)){
						case -1:
							result = FAILED_STOP;
						break;
						case 0:
							result = DELAYED;
						break;
						case 1:
							result = appendToFileW(req, nodePtr, threadId, APPEND_TO_FILE);
						break;
						}
					}
				}
			} else {
				result = appendToFileW(req, nodePtr, threadId, APPEND_TO_FILE);
			}
		break;
		case LOCK_FILE:
			printf("LOCK_FILE\n");
			if(req->sFileName == NULL){
				nodePtr = getFileFromSocket(req, &result, threadId);
				if(result == NONE){
					if(nodePtr == NULL){
						sendClientError(req->client, NO_SUCH_FILE);
						result = FAILED_STOP;
					} else {
						switch(tryUse(nodePtr, req, FALSE)){
						case -1:
							result = FAILED_STOP;
						break;
						case 0:
							result = DELAYED;
						break;
						case 1:
							result = lockFileW(req, nodePtr, threadId);
						break;
						}
					}
				}
			} else {
				result = lockFileW(req, nodePtr, threadId);
			}
		break;
		case UNLOCK_FILE:
			printf("UNLOCK_FILE\n");
			if(req->sFileName == NULL){
				nodePtr = getFileFromSocket(req, &result, threadId);
				if(result == NONE){
					if(nodePtr == NULL){
						sendClientError(req->client, NO_SUCH_FILE);
						result = FAILED_STOP;
					} else {
						switch(tryUse(nodePtr, req, FALSE)){
						case -1:
							result = FAILED_STOP;
						break;
						case 0:
							result = DELAYED;
						break;
						case 1:
							result = unlockFileW(req, nodePtr, threadId);
						break;
						}
					}
				}
			} else {
				result = unlockFileW(req, nodePtr, threadId);
			}
		break;
		case CLOSE_FILE:
			printf("CLOSE_FILE\n");
			if(req->sFileName == NULL){
				nodePtr = getFileFromSocket(req, &result, threadId);
				if(result == NONE){
					if(nodePtr == NULL){
						sendClientFatalError(req->client, NO_SUCH_FILE_F);
						result = FAILED_STOP;
					} else {
						/* file trovato (eseguo la richiesta) */
						switch(tryUse(nodePtr, req, FALSE)){
						case -1:
							result = FAILED_STOP;
						break;
						case 0:
							result = DELAYED;
						break;
						case 1:
							result = closeFileW(req, nodePtr, threadId);
						break;
						}
					}
				}
			} else {
				result = closeFileW(req, nodePtr, threadId);
			}
		break;
		case REMOVE_FILE:
			printf("REMOVE_FILE\n");
			if(req->sFileName == NULL){
				nodePtr = getFileFromSocket(req, &result, threadId);
				if(result == NONE){
					if(nodePtr == NULL){
						sendClientFatalError(req->client, NO_SUCH_FILE_F);
						result = FAILED_STOP;
					} else {
						/* file trovato (eseguo la richiesta) */
						switch(tryUse(nodePtr, req, FALSE)){
						case -1:
							result = FAILED_STOP;
						break;
						case 0:
							result = DELAYED;
						break;
						case 1:
							result = removeFileW(req, nodePtr, threadId);
						break;
						}
					}
				}
			} else {
				result = removeFileW(req, nodePtr, threadId);
			}
		break;
		default:
			printf("operazione sconosciuta ");
			printf("%d\n", GET_OP(req->oper));
			sendClientFatalError(req->client, NO_OP_SUPPORT);
			infoLog = newLogOp(GET_OP(req->oper), NULL, req->client, threadId, 0, 0, sizeof(int));
			LOG_INSERT(infoLog);
		break;
		}

		// sistemo per la prossima iterazione
		if( nodePtr != NULL && (result == FAILED_CONT || result == COMPLETED_CONT) ){
			/* devo continuare a lavorare */
			if( Pthread_mutex_lock( &(nodePtr->lock) ) != 0){
				perror("lockFile");
				infoLog = newLogOp(LOCK_FILE, req->sFileName, WORKER, threadId, 0, 0, 0);
				LOG_INSERT(infoLog);
				if(result == FAILED_CONT){
					result = FAILED_STOP;
				} else {
					result = COMPLETED_STOP;
				}
			} else {
				newReq = generalListPop(nodePtr->sFile->requestList);
				if(newReq == NULL){
					/* ho finito le richieste residue */
					nodePtr->sFile->flagUse = 0;
					if(result == FAILED_CONT){
						result = FAILED_STOP;
					} else {
						result = COMPLETED_STOP;
					}
				}
				Pthread_mutex_unlock( &(nodePtr->lock) );
				destroyRequest(&req);
				req = newReq;
			}
		}

	}while( req != NULL && (result == FAILED_CONT || result == COMPLETED_CONT) );
	
	// non credo si possa fare senza lock
	if(nodePtr != NULL && result != FILE_DELETED && result != DELAYED){
		if(startMutexTreeFile(fileStorage) == 0){
			if(Pthread_mutex_lock( &(nodePtr->lock) )){
				if(nodePtr->sFile != NULL && nodePtr->sFile->flagUse == 0){
					moveToFrontLRU(fileStorage, nodePtr);	
				}
				Pthread_mutex_unlock( &(nodePtr->lock) );
			}
			endMutexTreeFile(fileStorage);
		}
	}
	
	if(result == FILE_DELETED){
		/* mi occupo di informare gli altri client*/
		removeFileW(req, nodePtr, threadId);
	}
	
	
	printf("%d sono in vacanza\n", threadId);
	destroyRequest(&req);
	return;
}


// legge il nome di un file da un socket
// se ci riesce lo cerca nell'albero
// ritorna il puntatore al file
// se va a buon fine *result == NONE
TreeNode* getFileFromSocket(Request* req, enum operResult* result, int threadId){
	int dim = GET_PATH_DIM(req->oper);
	TreeNode* nodePtr = NULL;
	req->sFileName = malloc(dim + 1);
	*result = NONE;
	if(req->sFileName == NULL){
		/* non c'e' spazio */
		sendClientFatalError(req->client, NO_MEMORY_F);
		*result = FAILED_STOP;
	} else {
		/* c'e' spazio */
		if( readFormSocket(req->client, req->sFileName, dim + 1) == 0){
			/* tutto ok */
			printf("%d : nome file = %s\n", threadId, req->sFileName);////////////////////////////////////
			nodePtr = TreeFileFind(fileStorage, req->sFileName);
			if(errno != 0){
				sendClientError(req->client, UNKNOWN_ERROR);
				*result = FAILED_STOP;
			}
		} else {
			/* non ho letto il nome */
			*result = FAILED_STOP;
		}
	}
	return nodePtr;
}

int readFormSocket(int sock, void* buff, int dim){
	int buffInt;
	switch( readn(sock, buff, dim) ){
	case -1:
		/* impossible read */
		buffInt = IMPOSSIBLE_READ;
		writen(sock, &buffInt, sizeof(int));
		buffInt = sock;
		DISCONN_CLIENT(buffInt);
		return -1;
	break;
	case 0:
		/* client chiuso EOF */
		buffInt = sock;
		DISCONN_CLIENT(buffInt);
		return -1;
	break;
	default:
		return 0;
	break;
	}
}

int sendClientResult(int client, void* reply, int dim){
	if(writen(client, reply, dim) != 1){
		DISCONN_CLIENT(client);
		return -1;
	} else {
		LISTEN_CLIENT(client);
		return 0;
	}
}

void sendClientError(int sock, int err){
	int buffInt;
	if( writen(sock, &err, sizeof(int)) == 1){
		/* riesco a informare il client */
		buffInt = sock;
		LISTEN_CLIENT(buffInt);
	} else {
		/* non riesco a informare il client */
		buffInt = sock;
		DISCONN_CLIENT(buffInt);
	}
}

void sendClientFatalError(int sock, int err){
	int buffInt = err;
	writen(sock, &buffInt, sizeof(int));
	buffInt = sock;
	DISCONN_CLIENT(buffInt);
}


// ret -1 errore
// ret 0 delayed
// ret 1 utilizzabile
int tryUse(TreeNode* nodePtr, Request* req, int closeOnFail){
	
	if( Pthread_mutex_lock( &(nodePtr->lock)) != 0){
		if(closeOnFail){
			sendClientFatalError(req->client ,UNKNOWN_ERROR_F);
		} else {
			sendClientError(req->client, UNKNOWN_ERROR);
		}
		return -1;
	}
	printf("controllo se posso eseguire\n");
	if( nodePtr->sFile == NULL || nodePtr->sFile->flagUse || !fileUsePermitted(req->client, nodePtr->sFile) ){
		if( generalListInsert(req, nodePtr->sFile->requestList) == 0){
			/* operazione fallita (non ho messo in coda) */
			Pthread_mutex_unlock(&(nodePtr->lock));
			if(closeOnFail){
				sendClientFatalError(req->client ,UNKNOWN_ERROR_F);
			} else {
				sendClientError(req->client, UNKNOWN_ERROR);
			}
			return -1;
		} else {
			printf("richiesta in coda\n");//////////////////////////////////////////////////////////
			/* richiesta messa in coda */
			Pthread_mutex_unlock(&(nodePtr->lock));
			return 0;
		}
	} else {
		/* il thread inizia a gestire il file */
		printf("inizio gestione file\n");/////////////////////////////////////////////////////////
		nodePtr->sFile->flagUse = 1;
		Pthread_mutex_unlock(&(nodePtr->lock));
		return 1;
	}
	
}

int fileUsePermitted(int client, ServerFile* filePtr){
	return filePtr->flagO_lock == 0 || filePtr->lockOwner == client;
}

		// preparo la prossima operazione
		// se il file e' loked non devo 
		// se sono il possessore della lock dopo la mia operazione il thread deve smettere
		// usare 'completed_stop' o come si chiama quando eseguo un' operazione
		// in mutua esclusione
int closeConnectionW(int client){
	return COMPLETED_STOP;
}


int openFileW(Request* req, TreeNode** nodePtrP){
	TreeNode* nodePtr = *nodePtrP;
	LogOp* logInfo;
	// esiste ma lo voglio creare
	if(nodePtr != NULL && GET_O_CREATE(req->oper)){
		sendClientError(req->client, FILE_ALREADY_EXISTS);
		return FAILED_CONT;
	}
	// non esiste e non ho messo O_Create
	if(nodePtr == NULL && !GET_O_CREATE(req->oper)){
		sendClientError(req->client, NO_SUCH_FILE);
		return FAILED_STOP;
	}
	// non esiste ma ho messo O_Create => creo
	if(nodePtr == NULL && GET_O_CREATE(req->oper)){
		ServerFile* filePtr = newServerFile(req->client, GET_O_LOCK(req->oper), req->sFileName);
		if(filePtr == NULL){
			if(errno == ENOMEM){
				sendClientFatalError(req->client, NO_MEMORY_F);
			} else {
				sendClientError(req->client, UNKNOWN_ERROR);
			}
			return FAILED_STOP;
		}
		/* metto in albero */
		nodePtr = newTreeNode(filePtr, req->sFileName);
		if(nodePtr == NULL){
			sendClientFatalError(req->client, NO_MEMORY_F);
			destroyServerFile(filePtr);
			return FAILED_STOP;
		}
		ServerFile* expelled = NULL;
		TreeNode* toRemove = TreeFileinsert(fileStorage, &nodePtr, &expelled);
		switch( errno ){
		case EEXIST:
			sendClientError(req->client, FILE_ALREADY_EXISTS);
			destroyTreeNode(nodePtr);
			return FAILED_STOP;
		break;
		case 0:;
			int res = SUCCESS;
			*nodePtrP = nodePtr;
			if(toRemove != NULL && expelled != NULL){
				logInfo = newLogOp(REMOVE_FILE, expelled->namePath, LRU_ALG, 0, 1, 0, 1);
				LOG_INSERT(logInfo);
				clientFileDel(toRemove, resourceTable, expelled);
				informClientDelete(expelled);
				destroyServerFile(expelled);
			}
			if(GET_O_LOCK(req->oper)){
				clientOpen(req->client, nodePtr, TRUE, resourceTable);
				if( Pthread_mutex_lock( &(nodePtr->lock) ) == 0){ 
					nodePtr->sFile->flagUse = 0;
					Pthread_mutex_unlock( &(nodePtr->lock) );
				}
				sendClientResult(req->client, &res, sizeof(int));
				return COMPLETED_STOP;
			}
			clientOpen(req->client, nodePtr, FALSE, resourceTable);
			sendClientResult(req->client, &res, sizeof(int));
			return COMPLETED_CONT;
		break;
		case ENOMEM:
			sendClientError(req->client, NO_MEMORY_F);
			destroyTreeNode(nodePtr);
			return FAILED_STOP;
		break;
		case EPERM:
			if(toRemove != NULL && expelled != NULL){
				logInfo = newLogOp(REMOVE_FILE, expelled->namePath, LRU_ALG,0, 1, 0, 1);
				LOG_INSERT(logInfo);
				clientFileDel(toRemove, resourceTable, expelled);
				informClientDelete(expelled);
				destroyServerFile(expelled);
			}
			sendClientError(req->client, UNKNOWN_ERROR);
			destroyTreeNode(nodePtr);
			return FAILED_STOP;
		break;
		default:
			sendClientError(req->client, UNKNOWN_ERROR);
			destroyTreeNode(nodePtr);
			return FAILED_STOP;
		break;
		}
	}
	// il file va solo aperto
	nodePtr->sFile->creator = -1;
	int* client_ = malloc(sizeof(int));
	if(client_ == NULL){
		sendClientFatalError(req->client, NO_MEMORY_F);
		return FAILED_CONT;
	}
	*client_ = req->client;
	// controllare che non lo abbia gia' aperto
	if(!isInGeneralList(client_, nodePtr->sFile->openList)){
		if(generalListInsert( (void* ) client_, nodePtr->sFile->openList) == 0){
			if(errno == ENOMEM){
				sendClientFatalError(req->client, NO_MEMORY_F);
			} else {
				sendClientError(req->client, UNKNOWN_ERROR);
			}
			free(client_);
			return FAILED_CONT;
		}
	}
	// chiedo la lock
	if(GET_O_LOCK(req->oper)){
		clientOpen(req->client, nodePtr, TRUE, resourceTable);
		if( Pthread_mutex_lock( &(nodePtr->lock) ) != 0){
			clientClose(req->client, nodePtr, TRUE, resourceTable);
			generalListRemove(client_, nodePtr->sFile->openList);
			sendClientError(req->client, UNKNOWN_ERROR);
			// free(client_);
			return FAILED_CONT;
		}
		nodePtr->sFile->flagO_lock = 1;
		nodePtr->sFile->flagUse = 0;
		nodePtr->sFile->lockOwner = req->client;
		int reply = SUCCESS;
		sendClientResult(req->client, &reply, sizeof(int));
		Pthread_mutex_unlock( &(nodePtr->lock) );
		return COMPLETED_STOP;
	}
	
	// era gia' locked
	int reply = SUCCESS;
	if(nodePtr->sFile->flagO_lock == 1){
		clientOpen(req->client, nodePtr, TRUE, resourceTable);
		if( Pthread_mutex_lock( &(nodePtr->lock) ) == 0){ 
			nodePtr->sFile->flagUse = 0;
			Pthread_mutex_unlock( &(nodePtr->lock) );
		}
		sendClientResult(req->client, &reply, sizeof(int));
		return COMPLETED_STOP;
	}
	clientOpen(req->client, nodePtr, FALSE, resourceTable);
	sendClientResult(req->client, &reply, sizeof(int));
	return COMPLETED_CONT;
	
}

int readFileW(Request* req, TreeNode *nodePtr, int threadId){
	LogOp* infoLog;
	if(!isInGeneralList( &(req->client), nodePtr->sFile->openList )){
		infoLog = newLogOp(READ_FILE, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(infoLog);
		sendClientError(req->client, FILE_NOT_OPEN);
		return FAILED_CONT;
	}
	char* buffRet = malloc(sizeof(int) + nodePtr->sFile->dim);
	if(buffRet == NULL){
		infoLog = newLogOp(READ_FILE, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(infoLog);
		if(nodePtr->sFile->flagO_lock == 1){
			if(Pthread_mutex_lock( &(nodePtr->lock) )){
				nodePtr->sFile->flagUse = 0;
				Pthread_mutex_unlock( &(nodePtr->lock) );
			}
			sendClientError(req->client, NO_MEMORY);
			return FAILED_STOP;
		} else {
			sendClientError(req->client, NO_MEMORY);
			return FAILED_CONT;
		}
	}
	int res = SUCCESS;
	memcpy(buffRet, &res, sizeof(int));
	memcpy(buffRet + sizeof(int), nodePtr->sFile->data, nodePtr->sFile->dim);
	
	infoLog = newLogOp(READ_FILE, req->sFileName, req->client, threadId, 1, 0, sizeof(int) + nodePtr->sFile->dim);
	LOG_INSERT(infoLog);
	if(nodePtr->sFile->flagO_lock == 1){
		if(Pthread_mutex_lock( &(nodePtr->lock) )){
			nodePtr->sFile->flagUse = 0;
			Pthread_mutex_unlock( &(nodePtr->lock) );
		}
		sendClientResult(req->client, buffRet, sizeof(int) + nodePtr->sFile->dim);
		return COMPLETED_STOP;
	} else {
		sendClientResult(req->client, buffRet, sizeof(int) + nodePtr->sFile->dim);
		return COMPLETED_CONT;
	}
}

int readNFilesW(Request* req, int threadId){
	// ritorna traslato di 24
	LogOp* infoLog;
	char* buff = NULL;
	int nFile = (req->oper);
	int dim = 0;
	buff = getNElement(&dim, fileStorage, &nFile);
	if(buff == NULL){
		infoLog = newLogOp(READ_N_FILES, NULL, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(infoLog);
		sendClientError(req->client, NO_MEMORY << 24);
	}
	
	int res;
	res = (SUCCESS << 24) | nFile;
	memcpy(buff, &res, dim);
	infoLog = newLogOp(READ_N_FILES, NULL, req->client, threadId, 1, 0, dim);
	LOG_INSERT(infoLog);
	sendClientResult(req->client, buff, dim);
	free(buff);
	return COMPLETED_STOP;
}


int writeFileW(Request* req, TreeNode* nodePtr, int threadId){

	// o ho lock o nessuno la ha
	if(nodePtr->sFile->flagO_lock == 1 && nodePtr->sFile->creator == req->client){
		return appendToFileW(req, nodePtr, threadId, WRITE_FILE);
	}
	nodePtr->sFile->creator = -1;
	LogOp* infoLog;
	int buffInt;
	char* buff;
	int retWork;
	int alreadyInform = 0;
	infoLog = newLogOp(WRITE_FILE, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
	LOG_INSERT(infoLog);
	
	if(readn(req->client, &buffInt, sizeof(int)) != 1){
		alreadyInform = 1;
		sendClientFatalError(req->client, IMPOSSIBLE_READ << 24);
	} else {
		buff = malloc(buffInt);
		if(buff == NULL){
			alreadyInform = 1;
			sendClientFatalError(req->client, NO_MEMORY_F << 24);
		} else {
			if(readn(req->client, buff, buffInt) != 1){
				alreadyInform = 1;
				sendClientFatalError(req->client, IMPOSSIBLE_READ << 24);
			}
			free(buff);
		}
		
	}

	if(nodePtr->sFile->flagO_lock == 1){
		if(Pthread_mutex_lock( &(nodePtr->lock) ) == 0){
			nodePtr->sFile->flagUse = 0;
			Pthread_mutex_unlock( &(nodePtr->lock) );
		}
		retWork = FAILED_STOP;
	} else {
		retWork = FAILED_CONT;
	}
	if(!alreadyInform){
		sendClientError(req->client, FILE_ALREADY_OPEN);
	}
	return retWork;
}

// ritorna un intero diviso in 2
int appendToFileW(Request* req, TreeNode* nodePtr, int threadId, int logOpKind){
	int size = 0;
	LogOp* logInfo;
	nodePtr->sFile->creator = -1;
	if(readFormSocket(req->client, &size, sizeof(int)) == -1){
		sendClientFatalError(req->client, IMPOSSIBLE_READ << 24);
		logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(logInfo);
		return FAILED_CONT;
	}
	req->forEdit = malloc(size*sizeof(char));
	if(req->forEdit == NULL){
		sendClientFatalError(req->client, NO_MEMORY_F << 24);
		logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(logInfo);
		return FAILED_CONT;
	}
	if(readFormSocket(req->client, req->forEdit, size ) == -1){
		sendClientFatalError(req->client, IMPOSSIBLE_READ << 24);
		logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(logInfo);
		return FAILED_CONT;
	}
	printf("ho letto %d lettere : %s\n", size, req->forEdit);
	if(!isInGeneralList(&(req->client), nodePtr->sFile->openList)){
		sendClientError(req->client, FILE_NOT_OPEN << 24);
		logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(logInfo);
		return FAILED_CONT;
	}
	
	if(startMutexTreeFile(fileStorage) != 0){
		logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(logInfo);
		if(nodePtr->sFile->flagO_lock == 1){
			if(Pthread_mutex_lock( &(nodePtr->lock) ) == 0){
				nodePtr->sFile->flagUse = 0;
				Pthread_mutex_unlock( &(nodePtr->lock) );
			}
			sendClientError(req->client, UNKNOWN_ERROR << 24);
			return FAILED_STOP;
		}
		sendClientError(req->client, UNKNOWN_ERROR << 24);
		return FAILED_CONT;
	}
	
	int numVic = 0;
	int dimVic = sizeof(int);
	char* toretVict = malloc(sizeof(int));
	if(toretVict == NULL){
		endMutexTreeFile(fileStorage);
		logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(logInfo);
		if(nodePtr->sFile->flagO_lock == 1){
			if(Pthread_mutex_lock( &(nodePtr->lock) ) == 0){
				nodePtr->sFile->flagUse = 0;
				Pthread_mutex_unlock( &(nodePtr->lock) );
			}
			sendClientError(req->client, NO_MEMORY << 24);
			return FAILED_STOP;
		}
		sendClientError(req->client, NO_MEMORY << 24);
		return FAILED_CONT;
	}
	memcpy(toretVict, &numVic, sizeof(int));
	printf("Controllo se devo fare spazio\n");
	if(fileStorage->filedim + size > fileStorage->maxFileDim){
		int missingSpace = fileStorage->filedim + size - fileStorage->maxFileDim;
		int dataDimFreed = 0;
		GeneralList* toRem = makeSpace(fileStorage, 0, missingSpace);
		// controllare se devo ritornare il contenuto
		printf("checkpoint 0 spazio richiesto %d\n", missingSpace);
		if(toRem == NULL){
			// int forCli = (NO_MEMORY << 24);
			// printf("invio %d\n", forCli);
			sendClientError(req->client, (NO_MEMORY << 24));
			logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
			LOG_INSERT(logInfo);
			free(toretVict);
			return FAILED_CONT;
		}
		
		char* newPointer = NULL;
		int oldDim = 0;
		// potrei liberare lo spazio solo alla fine e lasciare libero l'albero
		TreeNode* currVic = generalListPop(toRem);
		ServerFile* filePtr = NULL;
		int nameLen;
		printf("checkpoint 1\n");
		while(currVic != NULL){
			numVic++;
			dataDimFreed += currVic->sFile->dim;
			if(GET_DIR_SAVE(req->oper)){
				oldDim = dimVic;
				nameLen = strlen(currVic->sFile->namePath) + 1;
				dimVic += currVic->sFile->dim + 2*sizeof(int) + nameLen;
				
				newPointer = realloc(toretVict, dimVic);
				printf("checkpoint 2\n");
				if(newPointer == NULL){
					free(toretVict);
					generalListDestroy(toRem);
					endMutexTreeFile(fileStorage);
					// informa log di cosa ha fatto algoritmo LRU
					logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
					LOG_INSERT(logInfo);
					logInfo = newLogOp(0, 0, LRU_ALG, threadId, 0, dimVic, numVic);
					LOG_INSERT(logInfo);
					if(nodePtr->sFile->flagO_lock == 1){
						if(Pthread_mutex_lock( &(nodePtr->lock) ) == 0){
							nodePtr->sFile->flagUse = 0;
							Pthread_mutex_unlock( &(nodePtr->lock) );
						}
						sendClientError(req->client, NO_MEMORY << 24);
						return FAILED_STOP;
					}
					sendClientError(req->client, NO_MEMORY << 24);
					return FAILED_CONT;
				}
				toretVict = newPointer;
				// copio informazioni in una stringa
				printf("checkpoint 2.1 %d\n", nameLen);
				memcpy(toretVict + oldDim, &nameLen, sizeof(int));
				oldDim += sizeof(int);
				printf("checkpoint 2.2 %s\n", currVic->sFile->namePath);
				memcpy(toretVict + oldDim, currVic->sFile->namePath, nameLen);
				oldDim += nameLen;
				printf("checkpoint 2.3 %d\n", currVic->sFile->dim);
				memcpy(toretVict + oldDim, &currVic->sFile->dim, sizeof(int));
				oldDim += sizeof(int);
				printf("checkpoint 2.4 %s\n", currVic->sFile->data);
				memcpy(toretVict + oldDim, currVic->sFile->data, currVic->sFile->dim);
				printf("checkpoint 2.5\n");
				for(size_t i = 0; i < dimVic; i++){
					printf("%d %c\n", toretVict[i], toretVict[i]);
				}
				
			}
			printf("checkpoint 4\n");
			// rimuovo il file 
			if(Pthread_mutex_lock( &(currVic->lock) ) != 0){
				free(toretVict);
				generalListDestroy(toRem);
				endMutexTreeFile(fileStorage);
				// informare log di cosa ha fatto LRU
				logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
				LOG_INSERT(logInfo);
				logInfo = newLogOp(0, NULL, LRU_ALG, threadId, 0, dimVic, numVic);
				LOG_INSERT(logInfo);
				if(nodePtr->sFile->flagO_lock == 1){
					if(Pthread_mutex_lock( &(nodePtr->lock) ) == 0){
						nodePtr->sFile->flagUse = 0;
						Pthread_mutex_unlock( &(nodePtr->lock) );
					}
					sendClientError(req->client, NO_MEMORY << 24);
					return FAILED_STOP;
				}
				sendClientError(req->client, NO_MEMORY << 24);
				return FAILED_CONT;
			}
				removeFromLRU(fileStorage, currVic);
				filePtr = currVic->sFile;
				currVic->sFile = NULL;
			Pthread_mutex_unlock(&(currVic->lock));
			clientFileDel(currVic, resourceTable, filePtr);
			printf("checkpoint 5\n");
			fflush(stdout);
			informClientDelete(filePtr);
			printf("checkpoint 6\n");
			fflush(stdout);
			destroyServerFile(filePtr);
			printf("checkpoint 7\n");
			fflush(stdout);
			currVic = generalListPop(toRem);
		}
		generalListDestroy(toRem);
		printf("checkpoint 8\n");
		fflush(stdout);
		if(GET_DIR_SAVE(req->oper)){
			memcpy(toretVict + sizeof(char), &numVic + sizeof(char), 3*sizeof(char));
		}
		logInfo = newLogOp(0, NULL, LRU_ALG, threadId, 1, dataDimFreed, numVic);
		LOG_INSERT(logInfo);
	}
	printf("spazio fatto");
	if(nodePtr->sFile->data == NULL){
		// allocato nuovo
		nodePtr->sFile->data = req->forEdit;
		nodePtr->sFile->dim += size;
		req->forEdit = NULL;
	} else {
		// rialloco
		char* newPlaceData = NULL;
		int oldDim = nodePtr->sFile->dim;
		nodePtr->sFile->dim += size;
		newPlaceData = realloc(nodePtr->sFile->data, nodePtr->sFile->dim);
		if(newPlaceData == NULL){
			// non c'e' memoria
			endMutexTreeFile(fileStorage);
			char buff = NO_MEMORY;
			memcpy(toretVict, &buff, sizeof(char));
			if(nodePtr->sFile->flagO_lock == 1){
				if(Pthread_mutex_lock( &(nodePtr->lock) ) == 0){
					nodePtr->sFile->flagUse = 0;
					Pthread_mutex_unlock( &(nodePtr->lock) );
				}
				if(GET_DIR_SAVE(req->oper)){
					logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
					LOG_INSERT(logInfo);
					sendClientResult(req->client, toretVict, sizeof(int));
				} else {
					logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, dimVic);
					LOG_INSERT(logInfo);
					sendClientResult(req->client, toretVict, dimVic);
				}
				free(toretVict);
				return FAILED_STOP;
			}
			if(GET_DIR_SAVE(req->oper)){
				logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
				LOG_INSERT(logInfo);
				sendClientResult(req->client, toretVict, sizeof(int));
			} else {
				logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 0, 0, dimVic);
				LOG_INSERT(logInfo);
				sendClientResult(req->client, toretVict, dimVic);
			}
			free(toretVict);
			return FAILED_CONT;
		}
		nodePtr->sFile->data = newPlaceData;
		memcpy(nodePtr->sFile->data + oldDim, req->forEdit, size);
		// appiccico infondo
	}
	
	fileStorage->filedim += size;
	if(fileStorage->filedim > fileStorage->maxUsedSpace){
		fileStorage->maxUsedSpace = fileStorage->filedim;
	}
	
	endMutexTreeFile(fileStorage);
	int buff = ((SUCCESS << 24) | numVic);
	memcpy(toretVict, &buff, sizeof(int));
	// toretVict[0] = SUCCESS;
	if(!GET_DIR_SAVE(req->oper)){
		dimVic = sizeof(int);
	}
	printf("devo solo rispondere\n");
	
	sendClientResult(req->client, toretVict, dimVic);
	for(size_t i = 0; i < dimVic; i++){
		printf("%d %c\n", toretVict[i], toretVict[i]);
	}
	free(toretVict);
	logInfo = newLogOp(logOpKind, req->sFileName, req->client, threadId, 1, size, dimVic);
	LOG_INSERT(logInfo);

	if(nodePtr->sFile->flagO_lock == 1){
		if(Pthread_mutex_lock( &(nodePtr->lock) ) == 0){
			nodePtr->sFile->flagUse = 0;
			Pthread_mutex_unlock( &(nodePtr->lock) );
		}
		return COMPLETED_STOP;
	}
	return COMPLETED_CONT;
}


int lockFileW(Request* req, TreeNode* nodePtr, int threadId){
	LogOp* infoLog;
	if(!isInGeneralList( &(req->client), nodePtr->sFile->openList )){
		sendClientError(req->client, FILE_NOT_OPEN);
		infoLog = newLogOp(LOCK_FILE, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(infoLog);
		return FAILED_CONT;
	}
	
	if(Pthread_mutex_lock( &(nodePtr->lock) ) != 0){
		sendClientError(req->client, UNKNOWN_ERROR);
		infoLog = newLogOp(LOCK_FILE, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(infoLog);
		return FAILED_CONT;
	}
	nodePtr->sFile->flagO_lock = 1;
	nodePtr->sFile->lockOwner = req->client;
	nodePtr->sFile->flagUse = 0;
	Pthread_mutex_unlock( &(nodePtr->lock) );

	clientLock(req->client, nodePtr, resourceTable);
	int res = SUCCESS;
	sendClientResult(req->client, &res, sizeof(int));
	infoLog = newLogOp(LOCK_FILE, req->sFileName, req->client, threadId, 1, 0, sizeof(int));
	LOG_INSERT(infoLog);
	
	return COMPLETED_STOP;
}


int unlockFileW(Request* req, TreeNode* nodePtr, int threadId){
	LogOp* infoLog;
	if(!isInGeneralList( &(req->client), nodePtr->sFile->openList )){
		sendClientError(req->client, FILE_NOT_OPEN);
		infoLog = newLogOp(UNLOCK_FILE, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(infoLog);
		return FAILED_CONT;
	}
	nodePtr->sFile->creator = -1;

	if(Pthread_mutex_lock( &(nodePtr->lock) ) != 0){
		sendClientError(req->client, UNKNOWN_ERROR);
		infoLog = newLogOp(UNLOCK_FILE, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(infoLog);
		return FAILED_CONT;// continuo perche' tanto voleva liberare il file
	}
	nodePtr->sFile->flagO_lock = 0;
	nodePtr->sFile->lockOwner = -1;
	Pthread_mutex_unlock( &(nodePtr->lock) );
	
	clientUnlock(req->client, nodePtr, resourceTable);
	int res = SUCCESS;
	sendClientResult(req->client, &res, sizeof(int));
	infoLog = newLogOp(UNLOCK_FILE, req->sFileName, req->client, threadId, 1, 0, sizeof(int));
	LOG_INSERT(infoLog);
	
	return COMPLETED_CONT;
}

// se ho la lock la tolgo e inizio a gestire
int closeFileW(Request* req, TreeNode* nodePtr, int threadId){
	LogOp* infoLog;
	if(!isInGeneralList( &(req->client), nodePtr->sFile->openList )){
		infoLog = newLogOp(CLOSE_FILE, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(infoLog);
		sendClientError(req->client, FILE_NOT_OPEN);
		return FAILED_CONT;
	}
	nodePtr->sFile->creator = -1;
	generalListRemove( &(req->client), nodePtr->sFile->openList);
	if(nodePtr->sFile->flagO_lock == 1){
		if(Pthread_mutex_lock( &(nodePtr->lock) ) == 0){
			nodePtr->sFile->flagO_lock = 0;
			nodePtr->sFile->lockOwner = -1;
			Pthread_mutex_unlock( &(nodePtr->lock) );
		}
		clientClose(req->client, nodePtr, TRUE, resourceTable);
	} else {
		clientClose(req->client, nodePtr, FALSE, resourceTable);
	}
	
	infoLog = newLogOp(CLOSE_FILE, req->sFileName, req->client, threadId, 1, 0, sizeof(int));
	LOG_INSERT(infoLog);
	int res = SUCCESS;
	sendClientResult(req->client, &res, sizeof(int));
	return COMPLETED_CONT;
}


int removeFileW(Request* req, TreeNode* nodePtr, int threadId){
	LogOp* infoLog;
	
	if(nodePtr->sFile->flagO_lock == 0){
		infoLog = newLogOp(REMOVE_FILE, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(infoLog);
		sendClientError(req->client, FILE_NOT_OPEN);
		return FAILED_CONT;
	}
	ServerFile* filePtr = TreeFileRemove(fileStorage, nodePtr);
	if(filePtr == NULL){
		// non e' possibile
		infoLog = newLogOp(REMOVE_FILE, req->sFileName, req->client, threadId, 0, 0, sizeof(int));
		LOG_INSERT(infoLog);
		sendClientError(req->client, UNKNOWN_ERROR);
		return FAILED_CONT;
	}
	
	infoLog = newLogOp(REMOVE_FILE, req->sFileName, req->client, threadId, 1, -filePtr->dim, sizeof(int));
	LOG_INSERT(infoLog);
	clientFileDel(nodePtr, resourceTable, filePtr);
	informClientDelete(filePtr);
	destroyServerFile(filePtr);
	int res = SUCCESS;
	sendClientResult(req->client, &res, sizeof(int));
	return COMPLETED_STOP;
}

// informare il log?
void informClientDelete(ServerFile* filePtr){
	int op, buffInt;
	Request* req = generalListPop(filePtr->requestList);
	while(req != NULL){
		op = GET_OP(req->oper);
		// NOTA : sono presenti solo le operazioni che possono finire nella coda del file
		if(op == REMOVE_FILE || op == CLOSE_FILE){
			buffInt = SUCCESS;
			sendClientResult(req->client, &buffInt, sizeof(int));
		} else {
			if(op == OPEN_FILE || op == READ_FILE || op == LOCK_FILE || op == UNLOCK_FILE){
			/* operazione fallita, non serve pulire il socket */
			sendClientError(req->client, NO_SUCH_FILE);
			} else {
				if(op == APPEND_TO_FILE){
					/* operazione fallita, devo pulire il socket */
					if(readFormSocket(req->client, &(req->editDim), sizeof(int)) == 0){
						req->forEdit = malloc(req->editDim + 1);
						if(req->forEdit == NULL){
							sendClientFatalError(req->client, NO_MEMORY_F);
						} else {
							if(readFormSocket(req->client, req->forEdit, req->editDim + 1) == 0){
								sendClientError(req->client, NO_SUCH_FILE);
							}
						}
					}
				} else {
					printf("operazione nel posto sbagliato\n");
					printf("clinet %d, operazione %d\n", req->client, op);
				}
			}
		}
		destroyRequest(&req);
		req = generalListPop(filePtr->requestList);
	}
}

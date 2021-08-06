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


ServerInfo srvGen;
FifoList resourceQueue;
FifoList logQueue;
TreeFile* fileStorage;

void readConfig(char* indirizzo);
int initServer(void);

void* dispatcher(void);
// ritorna il massimo FD da ascoltare
int updatemax(fd_set set, int maxFD);

void* worker(void);
// esegue una richiesta
void manageRequest(Request* req);
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

void* worker(void){
	int* readSocK;
	readSocK = (int*) pop(&(srvGen.toServe));
	if(readSocK == NULL){
		pthread_exit(NULL);
	} else {
		int clId = *readSocK;
		int oper = 0;
		int ret = 0;
		switch( readn(clId, &oper, sizeof(int)) ){
		case -1:
			/* errore in lettura non risolvibile */			
			ret = IMPOSSIBLE_READ;
			CONN_MARK(clId, NOT_CONNECTED);
			writen(clId, &ret, sizeof(int));
			if( writen(srvGen.doneReq[1], clId, sizeof(int)) == -1){
				exit(EXIT_FAILURE);
			}
		break;
		case 0:
			/* il client ha chiuso il socket */
			CONN_MARK(clId, NOT_CONNECTED);
			/* informare */
			if( writen(srvGen.doneReq[1], clId, sizeof(int)) == -1){
				exit(EXIT_FAILURE);
			}
		break;
		default:
			Request* req = newRequest(oper, clId, NULL, 0, NULL);
			manageRequest(req);
		break;
		}
	} 
	
}

void manageRequest(Request* req){
	ServerFile* filePtr = NULL;
	Request* newReq = NULL;
	enum operResult result = NONE;
	do{
		switch(GET_OP(req->oper)){
		case CLOSE_CONNECTION:
			/* non dovrebbe arrivare qui' a seconda dell'implementazione */
			closeConnection(req->client);
			printf("CLOSE_CONNECTION\n");
		break;
		case OPEN_FILE:
			printf("OPEN_FILE\n");\
			if(req->sFileName != NULL){
				/* e' la prima volta che vediamo questa richiesta */
				int dim ;
				switch( readn(req->client, &dim, sizeof(int)) ){
					case -1:
					/* errore in lettura */
					// chiudo client e continuo operazioni su un file se ci sto lavorando
					break;
					case 0:
						/* client ha chiuso la connessione */
					break;
					default:
					break;
				}
				
			}
			openFile(req);
		break;
		case READ_FILE:
			printf("READ_FILE\n");
			readFile(req);
		break;
		case READ_N_FILES:
			printf("READ_N_FILES\n");
			readNFiles(req);
		break;
		case WRITE_FILE:
			printf("WRITE_FILE\n");
			writeFile(req);
		break;
		case APPEND_TO_FILE:
			printf("APPEND_TO_FILE\n");
			appendToFile(req);
		break;
		case LOCK_FILE:
			printf("LOCK_FILE\n");
			lockFileW(req);
		break;
		case UNLOCK_FILE:
			printf("UNLOCK_FILE\n");
			unlockFileW(req);
		break;
		case CLOSE_FILE:
			printf("CLOSE_FILE\n");
			closeFile(req);
		break;
		case REMOVE_FILE:
			printf("REMOVE_FILE\n");
			removeFile(req);
		break;
		default:
			printf("operazione sconosciuta\n");
		break;
		}

		if( filePtr == NULL && (result == FAILED_CONT|| result == COMPLETED_CONT) ){
			/*	ho fatto la prima operazione su un file ma 
				per qualche motivo non c'e'
				deve essere sbagliato qualcosa */
		}

	}while( result != FAILED_CONT && result != COMPLETED_CONT );
	
}

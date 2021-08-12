#include <stdio.h>
#include <errno.h>
//formatting
#include <string.h>
#include <stdlib.h>
//connection
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <utils.h>
#include <api.h>
#include <request.h>
// base
#include <time.h>


int sock;
//connect to the server
int openConnection(const char* sockname, int msec, const struct timespec abstime){

	//struttura
	//creo un socket (per ora non ci faccio nulla)
	SOCKET(sock);

	struct sockaddr_un sa;
	strncpy(sa.sun_path, sockname, UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;

	// va gestita con dei cicli
	// i timer vanno resettati
	
	while(connect(sock,(struct sockaddr*) &sa, sizeof(sa)) == -1){
		if (errno == ENOENT){
			// aspetta msec 
			printf("enoent\n");
			// controlla se abstime e' finito oppure usare un segnale
			if(1){
				;
			}
		} else {
			return -1;
		}
	}
	printf("connesso\n");
	return 1;
}

// non gestisce le espulsioni
// non controlla gli errori
int openFile(const char* pathname, int flags){
	char* request;
	int sizeofReq = 0;
	int oper;
	int strDim = strlen(pathname);
	sizeofReq = strDim + sizeof(int) + 1;
	request = malloc(sizeofReq + 1);
	if(request == NULL){
		return -1;
	}
	int result;

	SET_CLEAN(oper);
	SET_OP(oper, OPEN_FILE);
	if( (O_LOCK & flags) != 0){
		SET_O_LOCK(oper);
	}
	if( (O_CREATE & flags) != 0){
		SET_O_CREATE(oper);
	}
	// SET_DIR_SAVE(oper);
	SET_PATH_DIM(oper, strDim);
	
	memcpy(request, &oper, sizeof(int));
	memcpy(request + sizeof(int), pathname, strDim + 1);
	writen(sock, request, sizeofReq);
	readn(sock, &result, sizeof(int));
	printf("result = %d\n", result);
	return 0;
}

int closeConnection(){
	sock = 0;
	return 0;
}

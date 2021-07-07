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



int sock;
//connect to the server
int openConnection(const char* sockname, int msec, const struct timespec abstime){

	//struttura
	//creo un socket (per ora non ci faccio nulla)
	SOCKET(sock);

	struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;

	// va gestita con dei cicli
	// i timer vanno resettati
	while(connect(sockname,(struct sockaddr*) &sa, sizeof(sa)) == -1){
		if (errno = ENOENT){
			//aspetta msec 
			
			//controlla se abstime e' finito oppure usare un segnale
			if(1){
				;
			}
		} else {
			return -1;
		}
	}
	
}

int closeConnection(){
	sock = NULL;
}
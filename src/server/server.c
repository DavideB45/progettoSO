#include <stdio.h>
#include <stdlib.h>
#include "../utils/utils.c"
// #include <utils.h>

//connection
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>


#define SOCKNAME "../mysock"
#define UNIX_PATH_MAX 108

int main(void){

	unlink(SOCKNAME);
	int sock;
	SOCKET(sock);

	//assegno un indirizzo al socket
	//struttura generica specializzata di volta in volta
	struct sockaddr_un sa;
	memset(&sa, 0, sizeof(sa));
	//inizializzo con i valori giusti
	strncpy(sa.sun_path, SOCKNAME,UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;
				//cast neccessario		
	if( bind(sock,(struct sockaddr *) &sa, sizeof(sa) ) ){//creo socket
		perror("bind");
		exit(1);
	}
	//tiene un po' di persone in coda per non rifiuare 
	//se il server non e' subito pronto ad accettarle
	if(listen(sock,SOMAXCONN)){
		perror("listen");
		exit(1);
	}

	int newConn;
	//ritorna un fd del scket che useremo per comunicare
	newConn = accept(sock,NULL,0);
	if(newConn == -1){
		perror("accept");
	}

}
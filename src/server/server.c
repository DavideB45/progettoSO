#include <stdio.h>
#include <stdlib.h>

// selfmade
#include <utils.h>
#include <server.h>

// connection
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>

#define SOCKET_FD servGeneral.sockFD

ServerInfo servGeneral;

int readFileConfig(char*);
int initServer(void);
void dispatcher(void);


int main(int argc, char* argv[]){




	

}


int readFileConfig(char*);

int initServer(void){
	
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

}

void dispatcher(void){
	
	int maxFD = servGeneral.sockFD;
	int newConn;
	fd_set set;
	fd_set rdSet;
	FD_ZERO(&set);
	FD_SET(SOCKET_FD, &set);

	while(servGeneral.serverStatus == S_WORKING){
		rdSet = set;
		if(select(maxFD + 1, &rdSet, NULL, NULL, NULL) == -1){
			/* SELECT ERROR */
		}
		for(size_t fd = 0; fd <= maxFD; fd++){
			if(FD_ISSET(fd, &rdSet)){
				if(fd == SOCKET_FD){
					//ritorna un fd del scket che useremo per comunicare
					newConn = accept(SOCKET_FD ,NULL, 0);
					if(newConn == -1){
						perror("accept");
						
					}
				}
			}
			
		}
		
	}
	
	
}
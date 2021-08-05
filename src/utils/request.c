#include <stdio.h>
#include <stdlib.h>

#include <request.h>

// crea una struct richiesta con i parametri passati
Request* newRequest(int oper, int client, char* sFilename, char* forEdit){
	Request* req = malloc(sizeof(Request));
	if(req == NULL){
		perror(malloc);
		return NULL;
	}
	req->oper = oper;
	req->client = client;
	req->forEdit = forEdit;
	req->sFileName = sFilename;
	return req;
}

// libera la memoria usata da una richiesta
void destroyRequest(Request* req){
	free(req->sFileName);
	free(req->forEdit);
	free(req);
	req = NULL;
}

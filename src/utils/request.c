#include <stdio.h>
#include <stdlib.h>

#include <request.h>

// crea una struct richiesta con i parametri passati
Request* newRequest(int oper, int client, char* sFilename, int editDim, char* forEdit){
	Request* req = malloc(sizeof(Request));
	if(req == NULL){
		perror(malloc);
		return NULL;
	}
	req->oper = oper;
	req->client = client;
	req->sFileName = sFilename;
	req->editDim = editDim;
	req->forEdit = forEdit;

	return req;
}

// libera la memoria usata da una richiesta
void destroyRequest(Request* req){
	free(req->sFileName);
	free(req->forEdit);
	free(req);
	req = NULL;
}

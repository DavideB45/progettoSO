#include <stdio.h>
#include <stdlib.h>

#include <request.h>

// crea una struct richiesta con i parametri passati
Request* newRequest(int oper, int client, char* sFilename, int editDim, char* forEdit){
	Request* req = malloc(sizeof(Request));
	if(req == NULL){
		perror("malloc request");
		return NULL;
	}
	req->oper = oper;
	req->client = client;
	req->sFileName = sFilename;
	req->editDim = editDim;
	req->forEdit = forEdit;

	return req;
}

// restituisce una stringa che identifica l'operazione
char* operatToString(enum operat oper, int uppercase){
	switch(oper){
	case CLOSE_CONNECTION:
		if(uppercase){
			return "CLOSE CONNECTION";
		} else {
			return "close connection";
		}
	break;
	case OPEN_CONNECTION:
		if(uppercase){
			return "OPEN CONNECTION";
		} else {
			return "open connection";
		}
	break;
	case OPEN_FILE:
		if(uppercase){
			return "OPEN FILE";
		} else {
			return "open file";
		}
	break;
	case READ_FILE:
		if(uppercase){
			return "READ FILE";
		} else {
			return "read file";
		}
	break;
	case READ_N_FILES:
		if(uppercase){
			return "READ N FILES";
		} else {
			return "read n files";
		}
	break;
	case WRITE_FILE:
		if(uppercase){
			return "WRITE FILE";
		} else {
			return "write file";
		}
	break;
	case APPEND_TO_FILE:
		if(uppercase){
			return "APPEND TO FILE";
		} else {
			return "append to file";
		}
	break; 
	case LOCK_FILE:
		if(uppercase){
			return "LOCK FILE";
		} else {
			return "lock file";
		}
	break;
	case UNLOCK_FILE:
		if(uppercase){
			return "UNLOCK FILE";
		} else {
			return "unlock file";
		}
	break;
	case CLOSE_FILE:
		if(uppercase){
			return "CLOSE FILE";
		} else {
			return "close file";
		}
	break;
	case REMOVE_FILE:
		if(uppercase){
			return "REMOVE FILE";
		} else {
			return "remove file";
		}
	break;
	default:
		if(uppercase){
			return "UNKNOWN OP";
		} else {
			return "unknown op";
		}
		
	break;
	}
}

// libera la memoria usata da una richiesta
void destroyRequest(Request** req){
	if(req == NULL || *req == NULL){
		return;
	}
	
	free((*req)->sFileName);
	free((*req)->forEdit);
	free(*req);
	*req = NULL;
}

#ifndef REQUEST_H
#define REQUEST_H

#pragma once

enum operat {CLOSE_CONNECTION, 
			OPEN_FILE, 
			READ_FILE, 
			READ_N_FILES, 
			WRITE_FILE, 
			APPEND_TO_FILE, 
			LOCK_FILE,
			UNLOCK_FILE,
			CLOSE_FILE,
			REMOVE_FILE
			};

// inizializza la richiesta
#define SET_CLEAN(x)  x = 0
// max op = 31
#define SET_OP(x, op) x = x | (op << 27)


// hano significato diverso se non usato per operazioni di open
// setta il flag o_lock nella richiesta
#define SET_O_LOCK(x) x = x | (1 << 26)
// setta il flag o_create
#define SET_O_CREATE(x) x = x | (1 << 25)

// hano significato diverso se non usato per operazioni che causano espulsioni di file
// setta il flag per salvare i file espulsi in una directory
#define SET_DIR_SAVE(x) x = x | (1 << 26)
// setta bit per salvare il path competo nella directory
#define SET_COMP_PATH(x) x = x | (1 << 25)

// setta l'operazione come non bloccante
#define SET_O_NON_BOCK(x) x = x | (1 << 24)

// max dim 4093
// scrive la dimensione del nome del file
#define SET_PATH_DIM(x, dim) x = x | (dim << 12)
// scrive la dimensione del nome della dir di salvataggio
#define SET_DIR_DIM(x, dim) x = x | dim // credo non abbia senso


typedef struct Request{
	enum operat type;
	int client;
	int O_lock;
	int sendDeleted;
	char* sFileName;
	char* forEdit;
}Request;

/* 
	idee per semplificare struct
	.. creare un unico int per O_lock,O_create,type,sendDeleted
	.. vedere se ci sta anche client 
*/
Request* newRequest(int type, int client, char* sFilename, char* forEdit, int O_lock, int sendDeleted);

void destroyRequest(Request req);

#endif
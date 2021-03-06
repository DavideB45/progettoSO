#ifndef REQUEST_H
#define REQUEST_H

#pragma once

enum operat {
	CLOSE_CONNECTION,
	OPEN_CONNECTION,
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

enum servErrorRet {
	NORET,
	SUCCESS,// operazione completata
	IMPOSSIBLE_READ,// fatale
	NO_OP_SUPPORT,// fatale
	INVALID_DIM,// fatale
	NO_MEMORY_F,// fatale
	NO_SUCH_FILE_F,// fatale
	UNKNOWN_ERROR_F,// fatale
	FILE_NOT_OPEN_F,// fatale
	SERVER_FULL,// fatale
	NO_MEMORY,// non fatale
	FILE_ALREADY_OPEN,// non fatale
	FILE_NOT_OPEN,// non fatale
	NO_SUCH_FILE,// non fatale
	UNKNOWN_ERROR,// non fatale
	FILE_ALREADY_EXISTS// non fatale
};



// inizializza la richiesta
#define SET_CLEAN(x)  x = 0

// max op = 31
#define SET_OP(x, op) x = x | (op << 27)
#define GET_OP(x) (x >> 27)

// hano significato diverso se non usato per operazioni di open
// setta il flag o_lock nella richiesta
#define SET_O_LOCK(x) x = x | (1 << 26)
// setta il flag o_create
#define SET_O_CREATE(x) x = x | (1 << 25)
// ritorna 1 se bisogna settare la lock
#define GET_O_LOCK(x) ((x & (1 << 26)) == (1 << 26))
// ritorna 1 se dobbiamo creare il file
#define GET_O_CREATE(x) ((x & (1 << 25)) == (1 << 25))

// hano significato diverso se non usato per operazioni che causano espulsioni di file
// setta il flag per salvare i file espulsi in una directory
#define SET_DIR_SAVE(x) x = x | (1 << 24)
// setta bit per salvare il path competo nella directory
#define SET_COMP_PATH(x) x = x | (1 << 23)
// ritorna 1 se opz salvataggio attiva
#define GET_DIR_SAVE(x) ((x & (1 << 24)) == (1 << 24))
// dice se salvare il path completo
#define GET_COMP_PATH(x) ((x & (1 << 23)) == (1 << 23))


// scrive la dimensione del nome del file 
// non conta lo "\\0"
#define SET_PATH_DIM(x, dim) x = x | (dim)
#define GET_PATH_DIM(x) (x & 0x003fffff)

#define SET_N_FILE(x, dim) x = x | (dim)
#define GET_N_FILE(x) (x & 0x003fffff)

typedef struct Request{
	int oper;
	int client;
	char* sFileName;
	int editDim;// non contiene '\0'
	char* forEdit;//da usare es per append
}Request;

// crea una struct richiesta con i parametri passati
// ritorna NULL se fallisce
Request* newRequest(int oper, int client, char* sFilename, int editDim, char* forEdit);

// restituisce una stringa che identifica l'operazione
char* operatToString(enum operat oper, int uppercase);

// libera la memoria usata da una richiesta
// e mette a NULL il puntatore?
void destroyRequest(Request** req);

#endif
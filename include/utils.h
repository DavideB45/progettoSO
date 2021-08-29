#ifndef UTILS_H
#define UTILS_H

#pragma once


#define SOCKET(x) x = socket(AF_UNIX, SOCK_STREAM,0);\
						if(x==-1){ \
							perror("create sock");\
							exit(1);\
						}

/*
	funzioni readn e writen
	riprese dalla soluzione
	di un' esercitazione
*/

// Evita letture parziali
// -1   errore (errno settato)
//  0   se durante la lettura da fd leggo EOF
//  1   se termina con successo
int readn(long fd, void *buf, size_t size);

// Evita scritture parziali
// -1   errore (errno settato)
//  0   se durante la scrittura la write ritorna 0
//  1   se la scrittura termina con successo
int writen(long fd, void *buf, size_t size);

// ritorna una copia della stringa toDup
char* duplicateString(const char* toDup);

int intCompare(const void* A,const void* B);

/*retun 0 on success else -1*/
int Pthread_mutex_init(pthread_mutex_t *mutex);

/*retun 0 on success else -1*/
int Pthread_mutex_lock(pthread_mutex_t *mutex);

/*retun 0 on success else -1*/
int Pthread_mutex_unlock(pthread_mutex_t *mutex);

#endif
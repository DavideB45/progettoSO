/*
	funzioni readn e writen
	copiate dalla soluzione
	di un' esercitazione
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

int intCompare(const void* A,const void* B){
	return *((int*) A) - *((int*) B);
}

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval  1 se termina con successo
 */
static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
		if ((r=read((int)fd ,bufptr,left)) == -1) {
			if (errno == EINTR) 
				continue;
			return -1;
		}
		if (r == 0) 
			return 0;   // EOF
		
		left    -= r;
		bufptr  += r;
    }
    return 1;
}

/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
		if ((r=write((int)fd ,bufptr,left)) == -1) {
			if (errno == EINTR) 
				continue;
			return -1;
		}
		if (r == 0) 
			return 0;  

		left    -= r;
		bufptr  += r;
	}
    return 1;
}


/*retun 0 on success else -1*/
int Pthread_mutex_init(pthread_mutex_t *mutex){
	pthread_mutexattr_t att;
	if(pthread_mutexattr_init(&att) != 0){
		return -1;
	}
	if(pthread_mutexattr_settype(&att, PTHREAD_MUTEX_ERRORCHECK) != 0){
		pthread_mutexattr_destroy(&att);
		return -1;
	}
	int ret = pthread_mutex_init(mutex, &att);
	if(ret == 0 || ret == EBUSY){
		pthread_mutexattr_destroy(&att);
		return 0;
	} else {
		pthread_mutexattr_destroy(&att);
		return -1;
	}
}

/*retun 0 on success else -1*/
int Pthread_mutex_lock(pthread_mutex_t *mutex){
	if(mutex == NULL){
		return -1;
	}
	
	int ret = pthread_mutex_lock(mutex);
	switch(ret){
	case EINVAL:
		if(Pthread_mutex_init(mutex) != 0){
			return -1;
		}
		ret = pthread_mutex_lock(mutex);
		if(ret == EDEADLK || ret == 0){
			return 0;
		}else {
			return -1;
		}
	case EDEADLK:
		/*mutex gia' acquisita*/
		return 0;
	default:
		if(ret == 0){
			return 0;
		} else {
			return -1;
		}
	}
}

/*retun 0 on success else -1*/
int Pthread_mutex_unlock(pthread_mutex_t *mutex){
	if(mutex == NULL){
		return -1;
	}
	
	int ret = pthread_mutex_unlock(mutex);
	

	if(ret == EINVAL){
		if( Pthread_mutex_init(mutex) != 0){
			return -1;
		}
		ret = pthread_mutex_unlock(mutex);	
	}
	
	if(ret == 0){
		return 0;
	}

	if(ret == EPERM){
		return -1;
	} else {
		exit(-1);
	}
}

#include <pthread.h>


void Pthread_mutex_lock(pthread_mutex_t *mutex){
	if(pthread_mutex_lock(mutex) != 0){
		perror("lock");
		exit(1);
	}
}
void Pthread_mutex_unlock(pthread_mutex_t *mutex){
	if(pthread_mutex_unlock(mutex) != 0){
		perror("unlock");
		exit(1);
	}
}
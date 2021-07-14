#ifndef UTILS_H
#define UTILS_H

#pragma once

#define SOCKET(x) x = socket(AF_UNIX, SOCK_STREAM,0);\
						if(x==-1){ \
							perror("create sock");\
							exit(1);\
						}

int intCompare(const void* A,const void* B);

/*retun 0 on success else -1*/
int Pthread_mutex_init(pthread_mutex_t *mutex);

/*retun 0 on success else -1*/
int Pthread_mutex_lock(pthread_mutex_t *mutex);

/*retun 0 on success else -1*/
int Pthread_mutex_unlock(pthread_mutex_t *mutex);

#endif
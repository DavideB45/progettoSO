#ifndef UTILS_H
#define UTILS_H

#pragma once

#define SOCKET(x) x = socket(AF_UNIX, SOCK_STREAM,0);\
						if(x==-1){ \
							perror("create sock");\
							exit(1);\
						}

int intCompare(void* A, void* B);
void Pthread_mutex_lock(pthread_mutex_t *mutex);
void Pthread_mutex_unlock(pthread_mutex_t *mutex);

#endif
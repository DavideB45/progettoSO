#ifndef FIFOLIST_H
#define FIFOLIST_H

#pragma once

#include <stdlib.h>
#include <pthread.h>

typedef struct FifoNode{
	void* data;
	struct FifoNode *nextPtr;
}FifoNode;

typedef struct FifoList{
	int dim;
	pthread_mutex_t lock;
	pthread_cond_t wait_to_read;
	FifoNode* headPtr;
	FifoNode* queuePtr;
}FifoList;

// create an empty list
FifoList* newList();

// return 1 if empty 0 if not empty
// -1 err
int isEmpty(FifoList* list);

// insert in queue
// ret -> 0 failure 1 success
_Bool insert(FifoList* list, void* elem);

// remove first element and return its data pointer
// if an error occurrs return NULL
void* pop(FifoList* list);

#endif
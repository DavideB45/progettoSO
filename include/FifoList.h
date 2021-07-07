#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <utils.h>

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

//create an empty list
FifoList* newList(){
	FifoList *list = (FifoList*) malloc(sizeof(FifoList));
	if(list == NULL){
		perror("list malloc");
		return NULL;
	}
	list->dim = 0;
	list->headPtr = NULL;
	list->queuePtr = NULL;
	pthread_mutex_init(&(list->lock), NULL);
	pthread_cond_init(&(list->wait_to_read), NULL);
	return list;
}

//return 1 if empty 0 if not empty
_Bool isEmpty(FifoList* list){
	return list->dim == 0;
}

//insert in queue
_Bool insert(FifoList* list, void* elem);
//remove first element and return its data pointer
void* pop(FifoList* list);

#endif
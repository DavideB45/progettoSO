#include <stdio.h>
// #include "../utils/utils.c"
#include <stdlib.h>
#include <errno.h>
#include <utils.h>
#include <pthread.h>
#include <files.h>
#include <request.h>

// int intCompare(void* A, void* B){
// 	return *((int*) A) - *((int*) B);
// }

char* testFile();

int main(void){
	printf("ciao\n");
	for (size_t i = 0; i < 10; i++){
		printf("%lu\t", i);
		free(NULL);
	}
	printf("\n");

	// pthread_mutex_t* mute = malloc(sizeof(pthread_mutex_t));
	// Pthread_mutex_init(mute);
	// Pthread_mutex_lock(mute);
	// Pthread_mutex_lock(mute);
	// Pthread_mutex_lock(mute);
	// free(mute);

	// int arr[10] = {12, 23, 3, 19, 19, 10, 0, 6, 54, 8};
	// qsort(arr, 10, sizeof(int), intCompare);
	// for (size_t i = 0; i < 10; i++){
	// 	printf("%d ", (int) arr[i]);
	// }printf("\n");
	
	// errno = EFAULT;
	// perror("test");

	// printf("\nresult: %s\n",testFile());

	return 0;
}


char* testFile(){
	ServerFile* cavia = newServerFile(1, 1);
	// inset/delete work
	int* palo;
	startMutex(cavia);
	for (size_t i = 0; i <= 10; i++){
		palo = malloc( sizeof(int) );
		*palo = i;
		addRequest(cavia, (Request*) palo);	
	}
	// while(palo = (int*) readRequest(cavia), palo != NULL){
	// 	printf("%d ", *palo);
	// 	free(palo);
	// }
	endMutex(cavia);

	// lock/unlock
	printf("u  1 %d\n", unlockFile(cavia, 1));
	printf("l 10 %d\n", lockFile(cavia, 10));
	printf("l 12 %d\n", lockFile(cavia, 12));
	printf("l 10 %d\n", lockFile(cavia, 10));
	printf("u 12 %d\n", unlockFile(cavia, 12));
	printf("u 10 %d\n", unlockFile(cavia, 10));
	printf("l 12 %d\n", lockFile(cavia, 12));
	destroyServerFile(cavia);
	return "fine";
	


}
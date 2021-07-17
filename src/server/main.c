#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>


#include <pthread.h>

#include <files.h>
#include <utils.h>
#include <request.h>
#include <main.h>






void read_config(char*);
// char* testFile();

int main(void){
	printf("ciao\n");
	for (size_t i = 0; i < 10; i++){
		printf("%lu\t", i);
		free(NULL);
	}
	printf("\n");
	read_config("./serv_info/file_config");
	// int fd;
	// printf("value real open\n");
	// fd = MAGIC_MARK(3, CLOSED);
	// printf("%5d %4d %4d\n", fd, GET_REAL(fd), IS_TO_RESET(fd));
	// fd = MAGIC_MARK(3, OPENED);
	// printf("%5d %4d %4d\n", fd, GET_REAL(fd), IS_TO_RESET(fd));
	// fd = MAGIC_MARK(4, CLOSED);
	// printf("%5d %4d %4d\n", fd, GET_REAL(fd), IS_TO_RESET(fd));
	// fd = MAGIC_MARK(4, OPENED);
	// printf("%5d %4d %4d\n", fd, GET_REAL(fd), IS_TO_RESET(fd));

	// pthread_mutex_t* mute = malloc(sizeof(pthread_mutex_t));
	// Pthread_mutex_init(mute);
	// Pthread_mutex_lock(NULL);
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


// char* testFile(){
// 	ServerFile* cavia = newServerFile(1, 1);
// 	// inset/delete work
// 	int* palo;
// 	startMutex(cavia);
// 	for (size_t i = 0; i <= 10; i++){
// 		palo = malloc( sizeof(int) );
// 		*palo = i;
// 		addRequest(cavia, (Request*) palo);	
// 	}
// 	// while(palo = (int*) readRequest(cavia), palo != NULL){
// 	// 	printf("%d ", *palo);
// 	// 	free(palo);
// 	// }
// 	endMutex(cavia);

// 	// lock/unlock
// 	printf("u  1 %d\n", unlockFile(cavia, 1));
// 	printf("l 10 %d\n", lockFile(cavia, 10));
// 	printf("l 12 %d\n", lockFile(cavia, 12));
// 	printf("l 10 %d\n", lockFile(cavia, 10));
// 	printf("u 12 %d\n", unlockFile(cavia, 12));
// 	printf("u 10 %d\n", unlockFile(cavia, 10));
// 	printf("l 12 %d\n", lockFile(cavia, 12));
// 	destroyServerFile(cavia);
// 	return "fine";
	


// }


void read_config(char* indirizzo){
	
	// char* sockName = "default";
	// int maxFileNum = 0;
	// int maxFileDim = 0;
	// int nWorker    = 0;

	FILE * filePtr = NULL;
	filePtr = fopen(indirizzo, "r");
	if(filePtr == NULL){
		printf("fileNotFound\n");
		return;
	}
	int num;
	char str[20];
	fscanf(filePtr, "%*[_n_worker]%*[ :=\t]%d%*[ \n]", &num);
	printf("worker : %d\n", num);
	fscanf(filePtr, "%*[_max_file]%*[ :=\t]%d\n", &num);
	printf("maxFil : %d\n", num);
	fscanf(filePtr, "%*[_max_dim]%*[ :=\t]%d%*[\n MbBm]", &num);
	printf("maxDim : %d\n", num);
	fscanf(filePtr, "%*[_socket_name]%*[ :=\t]%s\n", str);
	printf("nameSo : %s\n", str);
	fscanf(filePtr, "%*[_file_log_name]%*[ :=\t]%s\n", str);
	printf("nameLo : %s\n", str);
}

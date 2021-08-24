#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <time.h>
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
		// printf("%lu\t", i);
		free(NULL);
	}
	printf("\n");
	
	char cose[100];
	scanf("%s", cose);

	printf("lunghezza = %d %s\n", strlen(cose), cose);
	

	// time_t now = time(NULL);
	// char* tempo = ctime( &now );
	// printf("%sl\n", tempo);
	// tempo = operatToString(3, 0);
	// printf("%s %+d | ", tempo, -2);
	// FILE* fPtr = fopen("./servWork/piccione", "a+");
	// printf("%d\n", fprintf(fPtr, "tre %d\n", 3));
	// read_config("./servWork/file_config");
	

	// Request* test = NULL;
	// printf("%x\n", test);
	// test = newRequest(3, 3, NULL, 0, NULL);
	// printf("%x\n", test);
	// destroyRequest(&test);
	// printf("%x\n", test);

	// unsigned int operazione;
	// SET_CLEAN(operazione);
	// SET_OP(operazione, LOCK_FILE);
	// SET_O_CREATE(operazione);
	// SET_O_NON_BLOCK(operazione);
	// SET_DIR_SAVE(operazione);

	// SET_PATH_DIM(operazione, 300);

	// printf("%u\n", GET_OP(operazione));
	// printf("%u\n", GET_O_CREATE(operazione));
	// printf("%u\n", GET_O_LOCK(operazione));
	// printf("%u\n", GET_DIR_SAVE(operazione));
	// printf("%u\n", GET_COMP_PATH(operazione));
	// printf("%u\n", GET_O_NON_BLOCK(operazione));
	// printf("%u\n", GET_PATH_DIM(operazione));

	// switch(GET_OP(operazione)){
	// case CLOSE_CONNECTION:
	// 	printf("CLOSE_CONNECTION\n");
	// break;
	// case OPEN_FILE:
	// 	printf("OPEN_FILE\n");
	// break;
	// case READ_FILE:
	// 	printf("READ_FILE\n");
	// break;
	// case READ_N_FILES:
	// 	printf("READ_N_FILES\n");
	// break;
	// case WRITE_FILE:
	// 	printf("WRITE_FILE\n");
	// break;
	// case APPEND_TO_FILE:
	// 	printf("APPEND_TO_FILE\n");
	// break;
	// case LOCK_FILE:
	// 	printf("LOCK_FILE\n");
	// break;
	// case UNLOCK_FILE:
	// 	printf("UNLOCK_FILE\n");
	// break;
	// case CLOSE_FILE:
	// 	printf("CLOSE_FILE\n");
	// break;
	// case REMOVE_FILE:
	// 	printf("REMOVE_FILE\n");
	// break;
	// default:
	// 	printf("operazione sconosciuta\n");
	// break;
	// }

	// CLOSE_CONNECTION, 
	// OPEN_FILE, 
	// READ_FILE, 
	// READ_N_FILES, 
	// WRITE_FILE, 
	// APPEND_TO_FILE, 
	// LOCK_FILE,
	// UNLOCK_FILE,
	// CLOSE_FILE,
	// REMOVE_FILE

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
	
	char* sockName = NULL;
	char* logFile = NULL;
	int maxFileNum = 10;
	int maxFileDim = 100;
	int nWorker    = 5;

	FILE * filePtr = NULL;
	filePtr = fopen(indirizzo, "r");
	if(filePtr == NULL){
		printf("fileNotFound\n");
		return;
	}
	int num;
	char str[20];

	fscanf(filePtr, "%*[^_]");

	if(fscanf(filePtr, "%*[_n_worker]%*[ :=\t]%d%*[ \n]", &num) != 1){
		printf("default\n");
	} else {
		if(num > 0){
			nWorker = num;		
		}
	}
	printf("worker : %d\n", nWorker);

	if(fscanf(filePtr, "%*[_max_file]%*[ :=\t]%d\n", &num) != 1){
		printf("default\n");
	} else {
		if(num > 0){
			maxFileNum = num;
		}
	}
	printf("maxFil : %d\n", maxFileNum);

	if(fscanf(filePtr, "%*[_max_dim]%*[ :=\t]%d%*[\n MbBm]", &num) != 1){
		printf("default\n");
	} else {
		if(num > 0){
			maxFileDim = num;
		}
	}
	printf("maxDim : %d\n", maxFileDim);

	if(fscanf(filePtr, "%*[_socket_name]%*[ :=\t]%s\n", str) != 1){
		printf("default\n");
		str[0] = '0';
		str[1] = 0;
	}
	if( (strlen(str) == 1 && str[0] == '0') || (strcmp(str, "_file_log_name") == 0) ){
		printf("default\n");
		sockName = malloc(18);
		if(sockName == NULL)
			exit(1);
		strcpy(sockName, "./servWork/socket");
	} else {
		sockName = malloc( strlen(str) + 1 + 11 );
		if(sockName == NULL)
			exit(1);
		strcpy(sockName, "./servWork/");
		strcat(sockName, str);	
	}
	printf("nameSo : %s\n", sockName);

	if(fscanf(filePtr, "%*[_file_log_name]%*[ :=\t]%s\n", str) != 1){
		printf("default\n");
		str[0] = '0';
		str[1] = 0;
	}
	if(strlen(str) == 1 && str[0] == '0'){
		logFile = malloc(15);
		if(logFile == NULL)
			exit(1);
		strcpy(logFile, "./servWork/log");
	} else {
		logFile = malloc( strlen(str) + 1 + 11);
		if(logFile == NULL)
			exit(1);
		strcpy(logFile, "./servWork/");
		strcat(logFile, str);	
	}
	printf("nameLo : %s\n", logFile);
}

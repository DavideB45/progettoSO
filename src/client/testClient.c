#include <client.h>
#include <api.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void){
	struct timespec tempo;
	tempo.tv_nsec = 0;
	tempo.tv_sec = 10;
	openConnection("./servWork/daddu", 10, tempo);
	
	int flag = 0;
	int choise;
	flag = O_CREATE | O_LOCK;

	if(openFile("file/di/test2", flag) != 0){
		perror("open file/di/test2");
	}
	if(appendToFile("file/di/test2", "NANI", 5, NULL) != 0){
		perror("append file/di/test2");
	}
	if(openFile("peppino", flag) != 0){
		perror("open peppino");
	}
	if(openFile("pippo", flag) != 0){
		perror("open pippo");
	}
	if(openFile("pluto/gino", flag) != 0){
		perror("open pluto/gino");
	}
	if(appendToFile("peppino", "npersone", 9, NULL) != 0){
		perror("append peppino");
	}
	if(openFile("minni", flag) != 0){
		perror("open minni");
	}

	if(openFile("./servWork/log", O_LOCK | O_CREATE) != 0){
		perror("open ./servWork/log");
	}
	writeFile("./servWork/log", NULL);

	printf("scrivi per terminare [   ]\b\b\b");
	scanf("%d", &choise);
	
	
	sleep(2);
}
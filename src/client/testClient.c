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

	openFile("file/di/test2", flag);
	appendToFile("file/di/test2", "NANI", 5, NULL);
	openFile("peppino", flag);
	openFile("pippo", flag);
	openFile("pluto/gino", flag);
	appendToFile("peppino", "npersone", 9, NULL);
	openFile("minni", flag);

	printf("scrivi per terminare [   ]\b\b\b");
	scanf("%d", &choise);
	
	
	sleep(2);
}
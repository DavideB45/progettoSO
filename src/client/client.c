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

	printf("crea 1, apri 0 [   ]\b\b\b");
	scanf("%d", &choise);
	if(choise){
		flag = O_CREATE;
	}

	printf("Lock 1, noLock 0 [   ]\b\b\b");
	scanf("%d", &choise);
	if(choise){
		flag = flag | O_LOCK;
	}
	openFile("file/di/test", flag);
	
	sleep(2);
}
#include <client.h>
#include <api.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int main(void){
	struct timespec tempo;
	clock_gettime(CLOCK_REALTIME, &tempo);
	tempo.tv_sec += 10;
	close(-1);
	perror("close -1");
	openFile("giacomo", 0);
	perror("giacomo");
	openConnection("./servWork/daddu", 2000, tempo);
	perror("openConnection");
	closeConnection("./servWork/daddu");
	perror("closeConnection");


	int flag = 0;
	int choise;
	char* fileNome;
	char edit[200] = {0};
	while(1){
		printf("0: file/di/test\n1: test/di/file\n");
		scanf("%d", &choise);
		if(choise == 0){
			fileNome = "file/di/test";
		} else {
			fileNome = "test/di/file";
		}
		printf("0: apri/crea\n1: appendiaml\n");
		scanf("%d", &choise);
		if(choise == 0){
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
			openFile(fileNome, flag);
		} else {
			printf("scrivi una parola : ");
			scanf("%s", edit);
			if(appendToFile(fileNome, "edit dodici", 12, "./fileNonVoluti") != 0){
				perror("misfatto");
			}
		}
	}
	
	

	
	
	sleep(2);
}
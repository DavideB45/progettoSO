#include <client.h>
#include <api.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <time.h>

int multipleWrite(char* files, char* dirname, int printflag);

int main(int argc, char* argv[]){
	
	_Bool printFalg = 0;
	char* sockName = NULL;
	int opt, precOptInd;
    char* ends;
	char *optargDup;
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 0;

    while ( (opt = getopt(argc, argv, "hf:w:W:r:Rd:t:l:u:c:p")) != -1)
    {
        switch (opt){
        case 'h':
			printf(HARG);
			if(sockName != NULL){
				closeConnection(sockName);
				IF_PRINT(printFalg, perror("close connection"));
			}
			return 0;
        break;
        case 'f':
			if(sockName == NULL){
				sockName = optarg;
				struct timespec maxTime;
				clock_gettime(CLOCK_REALTIME, &maxTime);
				maxTime.tv_sec += 10;
				if(openConnection(sockName, 10, maxTime) != 0){
					if(printFalg)
						perror("open connection");
					return 0;
				}
				if(printFalg)
					perror("open connection");
				}	
        break;
        case 'w':
        break;
        case 'W':
			precOptInd = optind;
			optargDup = optarg;
			opt = getopt(argc, argv, "D:");
			if(opt == 'D'){// cambiare perche' prende un D qualunque
				multipleWrite(optargDup, optarg, printFalg);
			} else {
				multipleWrite(optargDup, NULL, printFalg);
				optind = precOptInd;
			}
        break;
		case 'D':
			printf("D\n");
		break;
		case 'r':
		break;
		case 'R':
			// printf("%s\n", argv[optind]);
		break;
		case 'd':
		break;
		case 't':;
			int msec = strtol(optarg, &ends, 10);
			if(*ends == '\0'){
				delay.tv_nsec = (msec % 1000)*1000000;
				delay.tv_sec = (msec/1000);
			} else {
				perror("strtol");
			}
		break;
		case 'l':
		break;
		case 'u':
		break;
		case 'c':
		break;
		case 'p':
			printFalg = 1;
		break;
		case '?':
		break;
		case ':':
		break;
        }
		nanosleep(&delay, NULL);
    }
    printf("FINE\n");
    return 0;
}

int multipleWrite(char* files, char* dirname, int printflag){

	char* fileName = strtok(files, ",");
	while( fileName != NULL){
		IF_PRINT(printflag, printf("write file %s\n", fileName));
		if(openFile(fileName, O_CREATE | O_LOCK) == 0){
			IF_PRINT(printflag, perror("create File"));
			writeFile(fileName, dirname);
			IF_PRINT(printflag, perror("write File"));
			closeFile(fileName);
			IF_PRINT(printflag, perror("close File"));
		}
		fileName = strtok(NULL, ",");
	}
	return 1;
}


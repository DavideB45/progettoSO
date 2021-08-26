#include <client.h>
#include <api.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>

int main(int argc, char* argv[]){
	
	_Bool printFalg = 0;
	char* sockName = NULL;
	int opt;
    char* ends;
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 0;

    while ( (opt = getopt(argc, argv, "hf:w:W:D:r:R+d:t:l:u:c:p")) != -1)
    {
        switch (opt){
        case 'h':
			printf(HARG);
			if(sockName != NULL){
				closeConnection(sockName);
				if(printFalg){
					perror("close connection");
				}
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
			// optindcosa utile per vedere se c'e' un -D
        break;
        case 'W':
        break;
        case 'D':
        break;
		case 'r':
		break;
		case 'R':
			printf("%s\n", argv[optind]);
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

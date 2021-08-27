#include <client.h>
#include <api.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

int dirWrite(char* srcDir, char* endDir, int printflag);
int multipleWrite(char* files, char* dirname, int printflag);

int multipleLock(char* files, int printflag);
int multipleUnLock(char* files, int printflag);

int main(int argc, char* argv[]){
	
	_Bool printFalg = 0;
	char* sockName = NULL;
	int opt, precOptInd;
    char* ends;
	char *optargDup;
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 0;

    while ( (opt = getopt(argc, argv, "hf:w:W:r:Rd:t:l:u:c:p")) != -1){
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
					IF_PRINT(printFalg, perror("open connection"));
					return 0;
				}
				IF_PRINT(printFalg, perror("open connection"));
			}
        break;
        case 'w':
			optargDup = optarg;
			if(optind < argc){
				if(argv[optind][0] == '-' && argv[optind][1] == 'D'){
					opt = getopt(argc, argv, "D:");
				}
			}
			if(opt == 'D'){
				dirWrite(optargDup, optarg, printFalg);
			} else {
				dirWrite(optargDup, NULL, printFalg);
			}
        break;
        case 'W':
			optargDup = optarg;
			// mettere degli if
			if(optind < argc){
				if(argv[optind][0] == '-' && argv[optind][1] == 'D'){
					opt = getopt(argc, argv, "D:");
				}
			}
			if(opt == 'D'){
				multipleWrite(optargDup, optarg, printFalg);
			} else {
				multipleWrite(optargDup, NULL, printFalg);
			}
        break;
		// case 'D':
		// 	printf("D\n");
		// break;
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
			enablePrint();
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

void recWatchDir(char* dir, char* endDir,long int* nFile, int printflag){
	
	chdir(dir);
	char cwd[256] = {0};
	if(getcwd(cwd, 256) == NULL){
		perror("no cwd");
		return;
	}
	int cwdDim = strlen(cwd);
	fflush(stdout);
	DIR* currdir = opendir(".");
	if(currdir == NULL){
		IF_PRINT(printflag, perror("openDir"))
		return;
	}
	struct dirent* dir_info = NULL;
	while( (dir_info = readdir(currdir)) != NULL  && *nFile != 0 ){
		if(dir_info->d_name[0] != '.'){
			if(dir_info->d_type == DT_DIR){
				recWatchDir(dir_info->d_name, endDir, nFile, printflag);
			} else {
				// mettere path completo
				cwd[cwdDim] = '/';
				memcpy(cwd + cwdDim + 1, dir_info->d_name, strlen(dir_info->d_name) + 1);
				if(openFile(cwd, O_CREATE | O_LOCK) == 0){
					(*nFile)--;
					IF_PRINT(printflag, printf("scrivo %s\n", cwd));
					if(writeFile(cwd, endDir) == 0){
						IF_PRINT(printflag, puts("success"));
						closeFile(cwd);
					} else {
						IF_PRINT(printflag, perror("write File"));
						closeFile(cwd);
					}
				} else {
					IF_PRINT(printflag, perror("create file"));
				}
			}
		}
	}
	chdir("..");
	closedir(currdir);
}

int dirWrite(char* srcDir, char* endDir, int printflag){
	
	char* tock = strtok(srcDir, ",");
	tock = strtok(NULL, ",");
	long int nFile = -1;
	if(tock != NULL){
		char* end;
		nFile = strtol(tock + 2, &end, 10);
		if(*end != '\0'){
			nFile = -1;
		}
	}
	int nStart = nFile;
	recWatchDir(srcDir, endDir, &nFile, printflag);
	if(nFile < 0){
		return -nFile + 1;
	} else {
		return nStart - nFile;
	}
}

int multipleWrite(char* files, char* dirname, int printflag){
	int numSucc = 0;
	char* fileName = strtok(files, ",");
	while( fileName != NULL){
		IF_PRINT(printflag, printf("lock file %s\n", fileName));
		if(openFile(fileName, O_CREATE | O_LOCK) == 0){
			if(writeFile(fileName, dirname) == 0){
				numSucc++;
			}
			IF_PRINT(printflag, perror("lock File"));
			closeFile(fileName);
		}
		fileName = strtok(NULL, ",");
	}
	return numSucc;
}

int multipleLock(char* files, int printflag){
	int numSucc;
	char* fileName = strtok(files, ",");
	while( fileName != NULL){
		IF_PRINT(printflag, printf("write file %s\n", fileName));
		if(openFile(fileName, O_CREATE | O_LOCK) == 0){
			if(lockFile(fileName) == 0){
				numSucc++;
			}
			IF_PRINT(printflag, perror("create File"));
		}
		fileName = strtok(NULL, ",");
	}
	return 1;
}

int multipleUnLock(char* files, int printflag){
	int numSucc;
	char* fileName = strtok(files, ",");
	while( fileName != NULL){
		IF_PRINT(printflag, printf("unlock file %s\n", fileName));
		if(unlockFile(fileName) == 0){
			numSucc++;
		}
		IF_PRINT(printflag, perror("unlock File"));
		closeFile(fileName);
		fileName = strtok(NULL, ",");
	}
	return numSucc;
}
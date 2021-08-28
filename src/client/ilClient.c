#include <client.h>
#include <api.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

int dirWrite(char* srcDir, char* endDir, int printflag);
int multipleWrite(char* files, char* dirname, int printflag);

int multipleRead(char* files, char* saveDir, int printflag);
int multipleRandomRead(char* N, char* saveDir, int printflag);

int multipleLock(char* files, int printflag);
int multipleUnLock(char* files, int printflag);

int multipleCancel(char* files, int printflag);

int main(int argc, char* argv[]){
	
	_Bool printFalg = 0;
	char* sockName = NULL;
	int opt;
    char* ends;
	char *optargDup;
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 0;

    while ( (opt = getopt(argc, argv, "hf:w:W:r:Rt:l:u:c:p")) != -1){
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
		// break;
		case 'r':
			optargDup = optarg;
			// mettere degli if
			if(optind < argc){
				if(argv[optind][0] == '-' && argv[optind][1] == 'd'){
					opt = getopt(argc, argv, "d:");
				}
			}
			if(opt == 'd'){
				multipleRead(optargDup, optarg, printFalg);
			} else {
				multipleRead(optargDup, NULL, printFalg);
			}
		break;
		case 'R':
			optargDup = optarg;
			// mettere degli if
			if(optind < argc){
				if(argv[optind][0] == '-' && argv[optind][1] == 'd'){
					opt = getopt(argc, argv, "d:");
					if(opt == 'd'){
						multipleRandomRead(NULL, optarg, printFalg);
					}
				} else {
					if(argv[optind][0] == 'n' && argv[optind][1] == '='){
						int numP = optind;
						if(optind + 1 < argc && argv[optind + 1][0] == '-' && argv[optind + 1][1] == 'd'){
							opt = getopt(argc, argv, "d:");
							if(opt == 'd'){
								multipleRandomRead(argv[numP] + 2, optarg, printFalg);
							}
						}
					}
				}
			}
		break;
		// case 'd':
		// break;
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
			multipleLock(optarg, printFalg);
		break;
		case 'u':
			multipleUnLock(optarg, printFalg);
		break;
		case 'c':
			multipleCancel(optarg, printFalg);
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
	if(sockName != NULL){
		closeConnection(sockName);
	}
	
    printf("FINE\n");
    return 0;
}


void recWatchDir(char* dir, char* endDir,long int* nFile, int printflag){
	
	chdir(dir);
	char cwd[MAXDIRDIM] = {0};
	if(getcwd(cwd, MAXDIRDIM) == NULL){
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
		IF_PRINT(printflag, printf("write file %s\n", fileName));
		if(openFile(fileName, O_CREATE | O_LOCK) == 0){
			if(writeFile(fileName, dirname) == 0){
				numSucc++;
			}
			IF_PRINT(printflag, perror("write File"));
			closeFile(fileName);
		}
		fileName = strtok(NULL, ",");
	}
	return numSucc;
}


void createParentDir(char* name){
	int divPos = 0;
	while(name[divPos] != '\0'){
		while(name[divPos] != '/' && name[divPos] != '\0'){
			divPos++;
		}
		if(name[divPos] != '\0'){
			name[divPos] = '\0';
			mkdir(name, 0777);
			name[divPos] = '/';
			divPos++;
		}
	}
}

int multipleRead(char* files, char* saveDir, int printflag){
	int numSucc;
	char* fileName = strtok(files, ",");
	char* buff;
	char cwd[MAXDIRDIM];
	FILE* filePtr;
	if(saveDir != NULL){
		if(getcwd(cwd, MAXDIRDIM) == NULL){
			return 0;
		}
		if(chdir(saveDir) != 0){
			return 0;
		}
	}
	size_t size;
	while( fileName != NULL){
		IF_PRINT(printflag, printf("read file %s\n", fileName));
		if(openFile(fileName, 0) == 0){
			if(readFile(fileName,(void**) &buff, &size) == 0){
				numSucc++;
				IF_PRINT(printflag, printf("read byte %ld\n", size));
				if(printflag && size < MAXPRINT){
					for(size_t i = 0; i < size; i++){
						printf("%c", buff[i]);
					}
					printf("\n");
				}
				if(saveDir != NULL){
					createParentDir(fileName + 1);
					filePtr = fopen(fileName + 1, "a+");
					if(filePtr != NULL){
						if(size > 0){
							fwrite(buff, 1, size, filePtr);
							IF_PRINT(printflag, perror("read File"));
						}
						fclose(filePtr);
					}
				} else {
					IF_PRINT(printflag, perror("read File"));
				}
				free(buff);
			}
		} else {
			IF_PRINT(printflag, perror("read File"));
		}
		fileName = strtok(NULL, ",");
	}
	chdir(cwd);
	return numSucc;
}

int multipleRandomRead(char* N, char* saveDir, int printflag){
	int readNum = -1;
	char* end;
	if(N != NULL){
		readNum = strtol(N, &end, 10);
		if(*end != '\0'){
			readNum = -1;
		}
	}
	readNum = readNFiles(readNum, saveDir);
	if(readNum >= 0 && printflag){
		printf("letti %d file\n", readNum);
	} else {
		IF_PRINT(printflag, perror("readN"));
	}
	return readNum;
}


int multipleLock(char* files, int printflag){
	int numSucc;
	char* fileName = strtok(files, ",");
	while( fileName != NULL){
		IF_PRINT(printflag, printf("lock file %s\n", fileName));
		if(openFile(fileName, O_LOCK) == 0){
			numSucc++;
		}
		IF_PRINT(printflag, perror("lock File"));
		fileName = strtok(NULL, ",");
	}
	return numSucc;
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


int multipleCancel(char* files, int printflag){
	int numSucc;
	char* fileName = strtok(files, ",");
	while( fileName != NULL){
		IF_PRINT(printflag, printf("cancel file %s\n", fileName));
		if(removeFile(fileName) == 0){
			numSucc++;
		}
		IF_PRINT(printflag, perror("cancel File"));
		fileName = strtok(NULL, ",");
	}
	return numSucc;
}

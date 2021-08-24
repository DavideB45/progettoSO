#include <stdio.h>
#include <errno.h>
//formatting
#include <string.h>
#include <stdlib.h>
//connection
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <utils.h>
#include <api.h>
#include <request.h>
// base
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>


int sock;

// 0 errore non fatale
// -1 errore fatale
static int setErrno(int error){
	
	switch (error){
	case NORET:
		errno = ESRCH;
		return -1;
	case SUCCESS:
		errno = 0;
		return 0;
	case IMPOSSIBLE_READ:
		errno = EFAULT;
		return -1;
	break;
	case NO_OP_SUPPORT:
		errno = EINVAL;
		return -1;
	case INVALID_DIM:
		errno = EINVAL;
		return -1;
	case NO_MEMORY_F:
		errno = ENOSPC;
		return -1;
	case NO_SUCH_FILE_F:
		errno = ENOENT;
		return -1;
	case UNKNOWN_ERROR_F:
		errno = EPERM;
		return -1;
	case FILE_NOT_OPEN_F:
		errno = EPERM;
		return -1;
	case SERVER_FULL:
		errno = ENOSPC;
		return -1;
	case NO_MEMORY:
		errno = ENOSPC;
		return 0;
	case FILE_ALREADY_OPEN:
		errno = EPERM;
		return 0;
	break;
	case FILE_NOT_OPEN:
		errno = EPERM;
		return 0;
	case NO_SUCH_FILE:
		errno = ENOENT;
		return 0;
	case UNKNOWN_ERROR:
		errno = EPERM;
		return 0;
	case FILE_ALREADY_EXISTS:
		errno = EEXIST;
		return 0;
	default:
		printf("boh error");
		return -1;
	break;
	}
}

static int readns(int fd, void* buff, ssize_t size){
	switch(readn(fd, buff, size)){
		case -1:
			/* errno settato */
			return -1;		
		case 0:
			close(sock);
			errno = ESRCH;
			return -1;
		default:
			errno = 0;
			return 0;
	}
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

static void saveExFile(int fd,const char* dirname, int num){
	if(num == 0){
		errno = 0;
		return;
	}
	
	int nameLen;
	int dataLen;
	FILE* filePtr;
	char* name;
	char* data;
	
	int currDir = open(".", O_DIRECTORY);
	if(currDir == -1){
		return;
	}
	
	if(chdir(dirname) != 0){
		if(errno == ENOENT){
			if(mkdir(dirname, 0777) != 0){
				int err = errno;
				close(currDir);
				errno = err;
				return;
			}
			if (chdir(dirname) != 0){
				int err = errno;
				close(currDir);
				errno = err;
				return;
			}
			
		} else {
			int err = errno;
			close(currDir);
			errno = err;
			return;
		}
	}
	for(size_t i = 0; i < num; i++){
		if(readns(fd, &nameLen, sizeof(int)) != 0){
			int err = errno;
			fchdir(currDir);
			close(currDir);
			errno = err;
			return;
		}
		printf("namelen = %d\n", nameLen);
		name = malloc(nameLen*sizeof(char));
		if(name == NULL){
			fchdir(currDir);
			close(currDir);
			errno = ENOMEM;
			return;
		}
		if(readns(fd, name, nameLen) != 0){
			int err = errno;
			free(name);
			fchdir(currDir);
			close(currDir);
			errno = err;
			return;
		}
		printf("name = %s\n",name);
		
		
		if(readns(fd, &dataLen, sizeof(int)) != 0){
			int err = errno;
			free(name);
			fchdir(currDir);
			close(currDir);
			errno = err;
			return;
		}
		printf("data dim = %d\n", dataLen);
		if(dataLen != 0){
			data = malloc(dataLen*sizeof(char));
			if(data == NULL){
				free(name);
				fchdir(currDir);
				close(currDir);
				errno = ENOMEM;;
				return;
			}
			
			if(readns(fd, data, dataLen) != 0){
				int err = errno;
				free(name);
				if(dataLen != 0){
					free(data);
				}
				fchdir(currDir);
				close(currDir);
				errno = err;
				return;
			}
		}
		
		
		
		// faccio le cos
		createParentDir(name);
		if(filePtr = fopen(name, "a+"), filePtr == 0){
			perror("open O_CREAT");
			if(dataLen != 0){
				free(data);
			}
			free(name);
			nameLen = 0;
			dataLen = 0;
			continue;
		}
		if(dataLen > 0){
			fwrite(data, sizeof(char), dataLen,filePtr);
		}
		fclose(filePtr);
		if(dataLen != 0){
			free(data);
		}
		free(name);
		nameLen = 0;
		dataLen = 0;
	}
	fchdir(currDir);
	close(currDir);
	errno = 0;
	return;
}

//connect to the server
int openConnection(const char* sockname, int msec, const struct timespec abstime){

	//struttura
	//creo un socket (per ora non ci faccio nulla)
	SOCKET(sock);

	struct sockaddr_un sa;
	strncpy(sa.sun_path, sockname, UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;

	// va gestita con dei cicli
	// i timer vanno resettati
	
	while(connect(sock,(struct sockaddr*) &sa, sizeof(sa)) == -1){
		if (errno == ENOENT){
			// aspetta msec 
			printf("enoent\n");
			// controlla se abstime e' finito oppure usare un segnale
			if(1){
				;
			}
		} else {
			return -1;
		}
	}
	printf("connesso\n");
	return 1;
}

// non gestisce le espulsioni
// non controlla gli errori
int openFile(const char* pathname, int flags){
	char* request;
	int sizeofReq = 0;
	int oper;
	int strDim = strlen(pathname);
	sizeofReq = strDim + sizeof(int) + 1;
	request = malloc(sizeofReq + 1);
	if(request == NULL){
		return -1;
	}
	int result;

	SET_CLEAN(oper);
	SET_OP(oper, OPEN_FILE);
	if( (O_LOCK & flags) != 0){
		SET_O_LOCK(oper);
	}
	if( (O_CREATE & flags) != 0){
		SET_O_CREATE(oper);
	}
	SET_PATH_DIM(oper, strDim);
	
	memcpy(request, &oper, sizeof(int));
	memcpy(request + sizeof(int), pathname, strDim + 1);
	writen(sock, request, sizeofReq);
	readn(sock, &result, sizeof(int));
	printf("result = %d\n", result);
	return 0;
}

int appendToFile(const char* pathname, void* buff, size_t size, const char* dirname){
	if(pathname == NULL || (size != 0 && buff == NULL) || size < 0){
		errno = EINVAL;
		return -1;
	}
	if(size == 0){
		errno = 0;
		return 0;
	}
	char* req;
	int op, reqDim;
	int nameLen = strlen(pathname);
	SET_CLEAN(op);
	SET_OP(op, APPEND_TO_FILE);
	SET_PATH_DIM(op, nameLen);
	if(dirname != NULL){
		SET_DIR_SAVE(op);
	}
	reqDim = 2*sizeof(int) + (nameLen + 1) + size;
	req = malloc(reqDim);
	if(req == NULL){
		errno = ENOMEM;
		return -1;
	}
	memcpy(req, &op, sizeof(int));
	memcpy(req + sizeof(int), pathname, nameLen + 1);
	memcpy(req + sizeof(int) + nameLen + 1, &size, sizeof(int));
	memcpy(req + 2*sizeof(int) + nameLen + 1, buff, size);

	int err;
	switch(writen(sock, req, reqDim)){
		case -1:
			err = errno;
			free(req);
			errno = err;
			return -1;
		break;
		case 0:
			free(req);
			close(sock);
			errno = ESRCH;
			return -1;
		break;
		case 1:
			free(req);
		break;
	}
	int result;
	switch(readn(sock, &result, sizeof(int))){
		case -1:
			// errno settato
			return -1;
		break;
		case 0:
			close(sock);
			errno = ESRCH;
			return -1;
		break;
		default:
			if(dirname != NULL){
				int nFile = result & 0x00ffffff;
				saveExFile(sock, dirname, nFile);
			}
			if(setErrno( result >> 24 ) == -1){
				return -1;
			}
			if((result >> 24) != 1){
				return -1;
			}
			return 0;
		break;
	}
}

int lockFile(const char* pathname){
	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}
	int op;
	int nameLen = strlen(pathname);
	char* req;
	SET_CLEAN(op);
	SET_OP(op, LOCK_FILE);
	SET_PATH_DIM(op, nameLen);
	req = malloc(sizeof(int) + nameLen + 1);
	if(req == NULL){
		errno = ENOMEM;
		return -1;
	}
	memcpy(req, &op, sizeof(int));
	memcpy(req + sizeof(int), pathname, nameLen + 1);
	
	int err;

	switch(writen(sock, req, sizeof(int) + nameLen + 1)){
	case -1:
		err = errno;
		free(req);
		errno = err;
	return -1;
	case 0:
		free(req);
		close(sock);
		errno = ESRCH;
	return -1;
	case 1:
		free(req);
	break;
	}

	int result = 0;
	if(readns(sock, &result, sizeof(int)) != 0){
		return -1;
	}
	if(result == SUCCESS){
		errno = 0;
		return 0;
	} else {
		if(setErrno(result) == -1){
			close(sock);
			setErrno(result);
		}
		return -1;
	}
}

int unlockFile(const char* pathname){
	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}
	int op;
	int nameLen = strlen(pathname);
	char* req;
	SET_CLEAN(op);
	SET_OP(op, UNLOCK_FILE);
	SET_PATH_DIM(op, nameLen);
	req = malloc(sizeof(int) + nameLen + 1);
	if(req == NULL){
		errno = ENOMEM;
		return -1;
	}
	memcpy(req, &op, sizeof(int));
	memcpy(req + sizeof(int), pathname, nameLen + 1);
	
	int err;

	switch(writen(sock, req, sizeof(int) + nameLen + 1)){
	case -1:
		err = errno;
		free(req);
		errno = err;
	return -1;
	case 0:
		free(req);
		close(sock);
		errno = ESRCH;
	return -1;
	case 1:
		free(req);
	break;
	}

	int result = 0;
	if(readns(sock, &result, sizeof(int)) != 0){
		return -1;
	}
	if(result == SUCCESS){
		errno = 0;
		return 0;
	} else {
		if(setErrno(result) == -1){
			close(sock);
			setErrno(result);
		}
		return -1;
	}
}

int closeConnection(){
	sock = 0;
	return 0;
}

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


int _sock = -1;
char* _sockName = NULL;
int resDim = 0;
int printInfo = 0;

#define IF_PRINT(print)	if(printInfo){\
							print;\
						}


///////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// UTILS //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

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
			errno = ESRCH;
			return -1;
		default:
			errno = 0;
			return 0;
	}
}

static void createParentDir(char* name){
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
		printf("nameLen = %d\n", nameLen);
		name = malloc(nameLen*sizeof(char) + 2);
		if(name == NULL){
			fchdir(currDir);
			close(currDir);
			errno = ENOMEM;
			return;
		}
		if(readns(fd, name + 2, nameLen) != 0){
			int err = errno;
			free(name);
			fchdir(currDir);
			close(currDir);
			errno = err;
			return;
		}
		
		
		if(readns(fd, &dataLen, sizeof(int)) != 0){
			int err = errno;
			free(name);
			fchdir(currDir);
			close(currDir);
			errno = err;
			return;
		}
		IF_PRINT( printf("byte download= %d\n", dataLen) );
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
		name[0] = '.';
		name[1] = '/';
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


///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////  API  //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

//connect to the server
int openConnection(const char* sockname, int msec, const struct timespec abstime){
	if(sockname == NULL){
		errno = EINVAL;
		return -1;
	}
	
	SOCKET(_sock);
	struct sockaddr_un sa;
	if(_sockName != NULL){
		free(_sockName);
	}
	_sockName = malloc(strlen(sockname) + 1);
	if(_sockName != NULL){
		strncpy(_sockName, sockname, strlen(sockname) + 1);
	}
	strncpy(sa.sun_path, sockname, UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;

	struct timespec sleepTime;
	sleepTime.tv_nsec = (msec % 1000)*1000000;
	sleepTime.tv_sec = (msec/1000);
	struct timespec currTime;
	
	while(connect(_sock,(struct sockaddr*) &sa, sizeof(sa)) == -1){
		if (errno == ENOENT || errno == EAGAIN || errno == EINTR || errno == ECONNREFUSED){
			clock_gettime(CLOCK_REALTIME, &currTime);
			if(currTime.tv_sec > abstime.tv_sec || \
				(currTime.tv_sec == abstime.tv_sec && currTime.tv_nsec >= abstime.tv_nsec)){
				errno = ETIME;
				return -1;
			}
			nanosleep(&sleepTime, NULL);
		} else {
			if(errno == EISCONN){
				errno = 0;
				return 0;
			}
			return -1;
		}
	}
	errno = 0;
	return 0;
}

int closeConnection(const char* sockname){
	if(sockname == NULL){
		errno = EINVAL;
		return -1;
	}
	if(_sockName != NULL){
		if(strcmp(_sockName, sockname) != 0){
			errno = EINVAL;
			return -1;
		}
	}
	if(close(_sock) == 0){
		_sock = -1;
		free(_sockName);
		_sockName = NULL;
		errno = 0;
		return 0;
	}
	// errno settato da close
	return -1;
}

// non gestisce le espulsioni
int openFile(const char* pathname, int flags){
	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}
	
	char* request;
	int sizeofReq = 0;
	int oper;
	int strDim = strlen(pathname);
	sizeofReq = strDim + sizeof(int) + 1;
	request = malloc(sizeofReq);
	if(request == NULL){
		return -1;
	}
	int result = 0;

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
	int err;
	switch(writen(_sock, request, sizeofReq)){
		case -1:
			err = errno;
			free(request);
			errno = err;
		return -1;
		case 0:
			free(request);
			errno = ESRCH;
			return -1;
		case 1:
			free(request);
		break;
	}
	if(readns(_sock, &result, sizeof(int)) == -1){
		return -1;
	}
	if(result != SUCCESS){
		setErrno(result);
		return -1;
	}
	errno = 0;
	return 0;
}

int closeFile(const char* pathname){
	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}
	int op;
	int result = 0;
	char* request;
	int nameLen = strlen(pathname);
	SET_CLEAN(op);
	SET_OP(op, CLOSE_FILE);
	SET_PATH_DIM(op, nameLen);
	request = malloc(sizeof(int) + nameLen + 1);
	if(request == NULL){
		errno = ENOMEM;
		return -1;
	}
	memcpy(request, &op, sizeof(int));
	memcpy(request + sizeof(int), pathname, nameLen + 1);
	
	int err;
	switch(writen(_sock, request, sizeof(int) + nameLen + 1)){
		case -1:
			err = errno;
			free(request);
			errno = err;
		return -1;
		case 0:
			free(request);
			errno = ESRCH;
		return -1;
		case 1:
			free(request);
		break;
	}
	if(readns(_sock, &result, sizeof(int)) == -1){
		return -1;
	}
	if(result != SUCCESS){
		setErrno(result);
		return -1;
	}
	errno = 0;
	return 0;
}

int readNFiles(int N, const char* dirname){
	if(dirname == NULL){
		errno = EINVAL;
		return -1;
	}
	if(N == 0){
		errno = 0;
		return 0;
	}
	int op;
	SET_CLEAN(op);
	SET_OP(op, READ_N_FILES);
	if(N < 0){
		N = 0;
	}
	SET_N_FILE(op, N);
	if(dirname != NULL){
		SET_DIR_SAVE(op);
	}
	switch(writen(_sock, &op, sizeof(int))){
		case -1:
			// errno settato
			return -1;
		case 0:
			errno = ESRCH;
			return -1;
		case 1:
		break;
	}
	int result;
	switch(readn(_sock, &result, sizeof(int))){
		case -1:
			// errno settato
			return -1;
		break;
		case 0:
			errno = ESRCH;
			return -1;
		break;
		default:
			if((result >> 24) == SUCCESS){
				int nFile = result & 0x00ffffff;
				saveExFile(_sock, dirname, nFile);
				if(errno != 0){
					return -1;
				}
				errno = 0;
				return nFile;
			} else {
				setErrno(result >> 24);
				return -1;
			}
		break;
	}
}

int writeFile(const char* pathname, const char* dirname){
	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}
	int fileFd = open(pathname, 0);
	if(fileFd == -1){
		return -1;
	}
	
	struct stat infoFile;
	if(fstat(fileFd, &infoFile) != 0){
		return -1;
	}
	char* fileDup = malloc(infoFile.st_size);
	if(fileDup == NULL){
		errno = ENOMEM;
		return -1;
	}
	int err;
	switch(readn(fileFd, fileDup, infoFile.st_size)){
		case -1:
			err = errno;
			close(fileFd);
			free(fileDup);
			errno = err;
		return -1;
		case 0:
			printf("hai letto EOF\n");
			err = errno;
			close(fileFd);
			free(fileDup);
			errno = err;
		return -1;
		case 1:
			close(fileFd);
		break;
	}
	char* req;
	int op, reqDim;
	int nameLen = strlen(pathname);
	SET_CLEAN(op);
	SET_OP(op, WRITE_FILE);
	SET_PATH_DIM(op, nameLen);
	if(dirname != NULL){
		SET_DIR_SAVE(op);
	}
	reqDim = 2*sizeof(int) + nameLen + 1 + infoFile.st_size;
	req = malloc(reqDim);
	if(req == NULL){
		free(fileDup);
		errno = ENOMEM;
		return -1;
	}
	memcpy(req, &op, sizeof(int));
	memcpy(req + sizeof(int), pathname, nameLen + 1);
	memcpy(req + sizeof(int) + nameLen + 1, &(infoFile.st_size), sizeof(int));
	memcpy(req + 2*sizeof(int) + nameLen + 1, fileDup, infoFile.st_size);
	free(fileDup);

	switch(writen(_sock, req, reqDim)){
		case -1:
			err = errno;
			free(req);
			errno = err;
			return -1;
		case 0:
			free(req);
			errno = ESRCH;
			return -1;
		case 1:
			IF_PRINT( printf("byte upload = %ld\n", infoFile.st_size) );
			free(req);
		break;
	}
	int result;
	switch(readn(_sock, &result, sizeof(int))){
		case -1:
			// errno settato
			return -1;
		break;
		case 0:
			errno = ESRCH;
			return -1;
		break;
		default:
			if(dirname != NULL){
				int nFile = result & 0x00ffffff;
				saveExFile(_sock, dirname, nFile);
				if(errno != 0){
					return -1;
				}
			}
			if((result >> 24) == SUCCESS){
				errno = 0;
				return 0;
			} else {
				setErrno(result >> 24);
				return -1;
			}
		break;
	}

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
	switch(writen(_sock, req, reqDim)){
		case -1:
			err = errno;
			free(req);
			errno = err;
			return -1;
		case 0:
			free(req);
			errno = ESRCH;
			return -1;
		case 1:
			free(req);
		break;
	}
	int result;
	switch(readn(_sock, &result, sizeof(int))){
		case -1:
			// errno settato
			return -1;
		break;
		case 0:
			errno = ESRCH;
			return -1;
		break;
		default:
			if(dirname != NULL){
				int nFile = result & 0x00ffffff;
				saveExFile(_sock, dirname, nFile);
				if(errno != 0){
					return -1;
				}
			}
			if((result >> 24) == SUCCESS){
				errno = 0;
				return 0;
			} else {
				setErrno(result >> 24);
				return -1;
			}
		break;
	}
}

int readFile(const char* pathname, void** buff, size_t* size){
	if(pathname == NULL || buff == NULL || size == NULL){
		errno = EINVAL;
		return -1;
	}
	int op;
	int nameLen = strlen(pathname);
	char* req;
	SET_CLEAN(op);
	SET_OP(op, READ_FILE);
	SET_PATH_DIM(op, nameLen);
	req = malloc(sizeof(int) + nameLen + 1);
	if(req == NULL){
		errno = ENOMEM;
		return -1;
	}
	memcpy(req, &op, sizeof(int));
	memcpy(req + sizeof(int), pathname, nameLen + 1);

	int err;

	switch(writen(_sock, req, sizeof(int) + nameLen + 1)){
	case -1:
		err = errno;
		free(req);
		errno = err;
	return -1;
	case 0:
		free(req);
		errno = ESRCH;
	return -1;
	case 1:
		free(req);
	break;
	}

	int result;
	if(readns(_sock, &result, sizeof(int)) != 0){
		return -1;
	}
	if(result != SUCCESS){
		setErrno(result);
		return -1;
	}
	*size = 0;
	if(readns(_sock, size, sizeof(int)) != 0){
		return -1;
	}
	*buff = malloc((*size)*sizeof(char));
	if(*buff == NULL){
		errno = ENOMEM;
		return -1;
	}
	if(readns(_sock, *buff, *size) != 0){
		free(*buff);
		return -1;
	}
	errno = 0;
	return 0;
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

	switch(writen(_sock, req, sizeof(int) + nameLen + 1)){
	case -1:
		err = errno;
		free(req);
		errno = err;
	return -1;
	case 0:
		free(req);
		errno = ESRCH;
	return -1;
	case 1:
		free(req);
	break;
	}

	int result = 0;
	if(readns(_sock, &result, sizeof(int)) != 0){
		return -1;
	}
	if(result == SUCCESS){
		errno = 0;
		return 0;
	} else {
		setErrno(result);
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

	switch(writen(_sock, req, sizeof(int) + nameLen + 1)){
	case -1:
		err = errno;
		free(req);
		errno = err;
	return -1;
	case 0:
		free(req);
		errno = ESRCH;
	return -1;
	case 1:
		free(req);
	break;
	}

	int result = 0;
	if(readns(_sock, &result, sizeof(int)) != 0){
		return -1;
	}
	if(result == SUCCESS){
		errno = 0;
		return 0;
	} else {
		setErrno(result);
		return -1;
	}
}

int removeFile(const char* pathname){
	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}
	int op;
	int nameLen = strlen(pathname);
	char* req;
	SET_CLEAN(op);
	SET_OP(op, REMOVE_FILE);
	SET_PATH_DIM(op, nameLen);
	req = malloc(sizeof(int) + nameLen + 1);
	if(req == NULL){
		errno = ENOMEM;
		return -1;
	}
	memcpy(req, &op, sizeof(int));
	memcpy(req + sizeof(int), pathname, nameLen + 1);
	
	int err;
	switch(writen(_sock, req, sizeof(int) + nameLen + 1)){
	case -1:
		err = errno;
		free(req);
		errno = err;
	return -1;
	case 0:
		free(req);
		errno = ESRCH;
	return -1;
	case 1:
		free(req);
	break;
	}

	int result = 0;
	if(readns(_sock, &result, sizeof(int)) != 0){
		return -1;
	}
	if(result == SUCCESS){
		errno = 0;
		return 0;
	} else {
		setErrno(result);
		return -1;
	}
}

void enablePrint(void){
	printInfo = 1;
}
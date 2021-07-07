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

#define SOCKNAME "../mysock"
#define UNIX_PATH_MAX 108
#define SOCKET(x) x = socket(AF_UNIX, SOCK_STREAM,0);\
						if(x==-1){ \
							perror("create sock");\
							exit(1);\
						}


int sock;
//connect to the server
int openConnection(const char* sockname, int msec, const struct timespec abstime);
int closeConnection();
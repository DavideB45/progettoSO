#ifndef FILES_H
#define FILES_H

#pragma once

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <generalList.h>



typedef int _Bool;

typedef struct ServerFile{
	pthread_mutex_t lock;
	_Bool flagUse;
	_Bool flagO_lock;
	int lockOwner;// cliend ID = Inode del coso per prlarci
	GeneralList* openList;// 
	GeneralList* requestList;// riempita da atri thread
	int dim;//capire quale deve essere l'unita' di misura
	char* data;
}ServerFile;

#endif
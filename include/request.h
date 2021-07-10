#ifndef REQUEST_H
#define REQUEST_H

#pragma once


typedef struct Request{
	int type;
	int client;
	int O_lock;
	int sendDeleted;
	char* sFileName;
	char* forEdit;
}Request;

/* 
	idee per semplificare struct
	.. creare un unico int per O_lock,O_create,type,sendDeleted
	.. vedere se ci sta anche client 
*/
Request* newRequest(int type, int client, char* sFilename, char* forEdit, int O_lock, int sendDeleted);

void destroyRequest(Request req);

#endif
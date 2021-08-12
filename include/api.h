#ifndef API_H
#define API_H

#pragma once

#include <stdlib.h>

#define SOCKNAME "../mysock"
#define UNIX_PATH_MAX 108

#define O_CREATE 2
#define O_LOCK 1

int sock;
//	connect to the server
int openConnection(const char* sockname, int msec, const struct timespec abstime);
/** apre un file, 
 * se chimata con flag O_CREATE lo crea, 
 * se chiamata con flag O_LOCK lo apre in mutua esclusione
 * \retval 0 se va a buon fine
 * \retval -1 se fallisce
 * \retval errno settato
 */
int openFile(const char* pathname, int flags);

int closeConnection();

#endif
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

/** apre il file PATHNAME 
 * se chimata con flag O_CREATE lo crea, 
 * se chiamata con flag O_LOCK lo apre in mutua esclusione
 * \retval 0 successo
 * \retval -1 se fallisce
 * \retval errno settato
 */
int openFile(const char* pathname, int flags);

/** appende i primi SIZE byte di BUFF al file PATHNAME
 * se DIRNAME non e' nullo viene usato per 
 * salvare eventuali file espulsi
 * \retval 0 successo
 * \retval -1 fallimento
 * \retval errno settato
 */
int appendToFile(const char* pathname, void* buff, size_t size, const char* dirname);


/** attiva la mutua esclusione sul file PATHNAME
 * \retval 0 successo
 * \retval -1 fallimento
 * \retval errno settato
 */
int lockFile(const char* pathname);

/** disattiva la mutua esclusione sul file PATHNAME
 * \retval 0 successo
 * \retval -1 fallimento
 * \retval errno settato
 */
int unlockFile(const char* pathname);

int closeConnection();

#endif
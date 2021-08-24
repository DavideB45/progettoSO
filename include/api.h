#ifndef API_H
#define API_H

#pragma once

#include <stdlib.h>

#define UNIX_PATH_MAX 108

#define O_CREATE 2
#define O_LOCK 1


/** apre una connessione con il server usando SOCKNAME
 * la richiesta viene ripetuta ogni MSEC millisecondi
 * fino al raggiungimento di ABSTIME
 * \retval 0 successo
 * \retval -1 se fallisce
 * \retval errno settato
 */
int openConnection(const char* sockname, int msec, const struct timespec abstime);

/** chiude la connessione con il server
 * SOCKNAME e' il nome del socket usato per comunicare
 * \retval 0 success
 * \retval -1 se fallisce
 * \retval errno settato
 */
int closeConnection(const char* sockname);

/** apre il file PATHNAME 
 * se chimata con flag O_CREATE lo crea, 
 * se chiamata con flag O_LOCK lo apre in mutua esclusione
 * \retval 0 successo
 * \retval -1 se fallisce
 * \retval errno settato
 */
int openFile(const char* pathname, int flags);

/** chiude il file PATHNAME
 * \retval 0 success
 * \retval -1 sefallisce
 * \retval errno settato
 */
int closeFile(const char* pathname);

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



#endif
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

/** legge al piu' N file dal server
 * e li salva in DIRNAME
 * se N <= 0 salva tutti i file disponibili
 * \retval numero di file letti ( >= 0)
 * \retval -1 fallimento
 * \retval setta errno
 */
int readNFiles(int N, const char* dirname);

/** scrive nel file(server) PATHNAME
 * il contenuto del file(locale) PATHNAME
 * ha successo se la chiamata precedente era
 * openFile(pathname, O_CREATE | O_LOCK)
 * se DIRNAME non e' nullo viene usato per
 * salvare eventuali file espulsi
 * \retval 0 successo
 * \retval -1 fallimento
 * \retval errno settato
 */
int writeFile(const char* pathname, const char* dirname);

/** appende i primi SIZE byte di BUFF al file PATHNAME
 * se DIRNAME non e' nullo viene usato per 
 * salvare eventuali file espulsi
 * \retval 0 successo
 * \retval -1 fallimento
 * \retval errno settato
 */
int appendToFile(const char* pathname, void* buff, size_t size, const char* dirname);

/** scrive il contenuto del file(sever) PATHNAME
 * in BUFF e salva la dimensione in SIZE
 * se non termina con successo buff e size non sono validi
 * \retval 0 successo
 * \retval -1 fallimento
 * \retval errno settato
 */
int readFile(const char* pathname, void** buff, size_t* size);

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

/** cancella il file PATHNAME dal server
 * l'operazione fallisce se non si ha 
 * la mutua esclusione suil file
 * \retval 0 successo
 * \retval -1 fallimento
 * \retval errno settato
 */
int removeFile(const char* pathname);

#endif
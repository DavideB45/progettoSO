#ifndef CLIENT_H
#define CLIENT_H

#pragma once

#define HARG 	"-h : stampa la lista di tutte le opzioni accettate dal client e termina immediatamente;\n\
-f filename : specifica il nome del socket AF_UNIX a cui connettersi;\n\
-w dirname[,n=0] : invia al server i file nella cartella ‘dirname’ finche'\n\
	si leggono ‘n‘ file; se n=0 (o non è specificato) non c’è un limite superiore\n\
-W file1[,file2]: lista di nomi di file da scrivere nel server separati da ‘,’;\n\
-D dirname : cartella dove vengono scritti i file che il server rimuove a\n\
	seguito di capacity misses causate dall’opzione ‘-w’ e ‘-W’. Se l’opzione ‘-D’ non \n\
	viene specificata, tutti i file che il server invia verso il client vengono buttati via.\n\
-r file1[,file2] : lista di nomi di file da leggere dal server\n\
-R [n=0] : tale opzione permette di leggere ‘n’ file qualsiasi \n\
	se n=0 (onon è specificato) allora vengono letti tutti i file presenti nel server;\n\
-d dirname : cartella dove scrivere i file letti dal server con l’opzione ‘-r’ o ‘-R’.\n\
	L’opzione -d va usata congiuntamente a ‘-r’ o ‘-R’\n\
	senza specificare l’opzione ‘-d’ i file letti non vengono memorizzati sul disco;\n\
-t time : tempo in millisecondi che intercorre tra l’invio di due richieste successive\n\
	al server (se non specificata si suppone -t 0)\n\
-l file1[,file2] : lista di nomi di file su cui acquisire la mutua esclusione;\n\
-u file1[,file2] : lista di nomi di file su cui rilasciare la mutua esclusione;\n\
-c file1[,file2] : lista di file da rimuovere dal server se presenti;\n\
-p : abilita le stampe sullo standard output per ogni operazione.\n"

#define IF_PRINT(flag, print)	if(flag){\
									print;\
								}

#endif
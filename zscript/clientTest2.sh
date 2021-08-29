#!/bin/bash

rm -r resTest
rm -r expelled
mkdir resTest
SLOWSETT=(-p -t 400 -f ./servWork/socket2)
FASTSETT=(-p -t 100 -f ./servWork/socket2)
DFIL="$PWD/fileXtestLRU"
DREM="$PWD/expelled"

#inserisce i 6 file contenuti nella directory fileXtestLRU/a
#testa l'algoritmo di espulsione senza interferenze
./bin/ilClient.out ${FASTSETT[@]} -w "$DFIL/a" -D "$DREM/cl1" -flush&>> resTest/cl1 &

sleep 2
#client che genera richieste in modo piu' lento
./bin/ilClient.out ${SLOWSETT[@]} -W "$DFIL/b/b/file1,$DFIL/b/b/file2,$DFIL/b/a/file3" -D "$DREM/cl2" -flush&>> resTest/cl2 &

./bin/ilClient.out ${FASTSETT[@]} -W "$DFIL/b/b/file3" -D "$DREM/cl3" -flush&>> resTest/cl3 &

./bin/ilClient.out ${FASTSETT[@]} -W "$DFIL/b/a/file1,$DFIL/b/a/file2" -D "$DREM/cl4"\
 -r "$DFIL/b/a/file1" -d "$DREM/cl4" -flush&>> resTest/cl4 &


#salva tutto cio' che e' disponibine in un momento casuale
./bin/ilClient.out ${FASTSETT[@]} -R -d "$DREM/cl5" -flush&>> resTest/cl5 &

#prova a inserire file che erano stati inseriti all'inizio dello script
./bin/ilClient.out ${SLOWSETT[@]} -W "$DFIL/a/a/file1,$DFIL/a/a/file3" -D "$DREM/cl6"\
 -r "$DFIL/a/a/file2" -d "$DREM/cl6" -flush&>> resTest/cl6 &

sleep 1

kill -HUP $(ps aux | grep server.out | tr -s " " | cut -d ' ' -f 2 | head -n 1)
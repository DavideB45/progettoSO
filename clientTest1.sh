#!/bin/bash

mkdir resTest
mkdir downloaded
SETT=(-t 200 -p -f ./servWork/socket1)
./bin/ilClient.out ${SETT[@]} -w "$PWD/filePerTest/a/a/,n=6" -r "$PWD/filePerTest/a/a/file1"\
 -d "$PWD/downloaded/by1" -l "$PWD/filePerTest/a/a/file1" -c "$PWD/filePerTest/a/a/file1" -flush&>> resTest/cl1 &

./bin/ilClient.out ${SETT[@]} -W "$PWD/filePerTest/b/e/file3,$PWD/filePerTest/d/c/file5"\
 -R n=4 -d "$PWD/downloaded/by2" -flush&>> resTest/cl2 &

./bin/ilClient.out ${SETT[@]} -w "$PWD/filePerTest/e/,n=9" -D "$PWD/downloaded/by3" -flush&>> resTest/cl3 &

./bin/ilClient.out ${SETT[@]} -l "$PWD/filePerTest/a/a/file3,$PWD/filePerTest/a/a/file4" -flush&>> resTest/cl4 &

./bin/ilClient.out ${SETT[@]} -W "$PWD/filePerTest/c/e/file3,$PWD/filePerTest/c/c/file5"\
 -l "$PWD/filePerTest/c/e/file3" \
 -u "$PWD/filePerTest/c/c/file5,$PWD/filePerTest/c/e/file3" -flush&>> resTest/cl5 &

sleep 1

kill -HUP $(ps aux | grep server.out | tr -s " " | cut -d ' ' -f 2 | head -n 1)

sleep 7
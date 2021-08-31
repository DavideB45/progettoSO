#!/bin/bash
echo
echo -e "\e[1;32mREAD WRITE\e[0m"
TOT=$(grep 'read' $1 | grep 'res = 1' | cut -d '=' -f 3 | cut -d ' ' -f 2 | paste -sd+ | bc)
NUM=$(grep 'read' $1 | grep 'res = 1' | wc -l)
MEDIA=$(echo "scale=2; ($TOT / $NUM)" | bc -l)
echo "N read = $NUM M dim = $MEDIA"

TOT=$(grep 'write' $1 | grep 'res = 1' | cut -d '=' -f 3 | cut -d '+' -f 2 | cut -d ' ' -f 1 | paste -sd+ | bc)
NUM=$(grep 'write' $1 | grep 'res = 1' | wc -l)
MEDIA=$(echo "scale=2; ($TOT / $NUM)" | bc -l)
echo "N writ = $NUM M dim = $MEDIA"

echo -e "\e[1;32mOPEN\e[0m"

NUM=$(grep 'open file' $1 | grep 'res = 1' | wc -l)
echo "N open = $NUM"
NUM=$(grep 'open file' $1 | grep 'locked' | grep 'res = 1' | wc -l)
echo "N O_lk = $NUM"
NUM=$(grep 'open file' $1 | grep 'created' | grep 'res = 1' | wc -l)
echo "N O_cr = $NUM"

echo -e "\e[1;32mLOCK\e[0m"

NUM=$(grep ' lock file' $1 | grep 'res = 1' | wc -l)
echo "N lock = $NUM"
NUM=$(grep 'unlock file' $1 | grep 'res = 1' | wc -l)
echo "N unlk = $NUM"
NUM=$(grep 'close file' $1 | grep 'res = 1' | wc -l)
echo "N clos = $NUM"

echo -e "\e[1;32mGENERAL\e[0m"

NUM=$(grep 'MAX file' $1 | cut -d '|' -f 1 | cut -d ' ' -f 4)
echo "MX fil = $NUM"
NUM=$(grep 'MAX dim' $1 | cut -d '|' -f 2 | cut -d ' ' -f 5)
NUM=$(echo "scale=3; ($NUM / 1000000)" | bc -l )
echo "MX dim = $NUM Mb"

NUM=$(grep 'LRU call' $1 | cut -d '|' -f 1 | cut -d ' ' -f 4) 
echo "LRU ex = $NUM"
NUM=$(grep 'MAX con' $1 | cut -d '|' -f 2 | cut -d ' ' -f 5)
echo "MX con = $NUM"

NUMTHREAD=$(grep ")" $1 | grep -v '-' | tr -s ' ' | cut -d ' ' -f 5 | cut -d ')' -f 1 | sort -n | tail -n 1)

echo -e "\e[1;32mTHREAD\e[0m"
echo Thread = $(($NUMTHREAD + 1))
for ((i=0;i<=$NUMTHREAD;i++)) do
	NUM=$(grep "$i)" $1 | grep -v '-' | wc -l)
	echo -e "thread \e[1;32m$i\e[0m ha eseguito \e[1;32m$NUM\e[0m operazioni"
done
echo
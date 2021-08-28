#!/bin/bash

DIRS=(a b)
NUMERI=(primo secondo terzo)
NOMIPERFILE=(file1 file2 file3)
rm -r "fileXtestLRU"

function riempifile(){
	for lettera in {a..z};do
		if (($RANDOM % 2));then
			echo -n $lettera >> ${NOMIPERFILE[y]}
		fi
	done
	echo line >> ${NOMIPERFILE[y]}
	for lettera in {a..z};do
		echo -n $lettera >> ${NOMIPERFILE[y]}
		echo -n $lettera >> ${NOMIPERFILE[y]}
	done
	echo "newline" >> ${NOMIPERFILE[y]}
	for ((z=0; z<25000; z++)) do
		echo -n "$z" >> ${NOMIPERFILE[y]}
		if ((y >= 1));then
			echo -n "$z" >> ${NOMIPERFILE[y]}
		fi
		if ((y >= 2));then
			echo -n "$z" >> ${NOMIPERFILE[y]}
		fi
		if !(($z % 100));then
			echo newline >> ${NOMIPERFILE[y]}
		fi
	done
}
echo inizio generazione file
mkdir "fileXtestLRU"
cd "fileXtestLRU"
for (( i=0; i<${#DIRS[@]}; i++ )) do
	mkdir ${DIRS[$i]}
	cd ${DIRS[$i]}
	for (( j=0; j<${#DIRS[@]}; j++ )) do
		mkdir ${DIRS[$j]}
		cd ${DIRS[$j]}
		for (( y=0; y<${#NUMERI[@]}; y++)) do
			echo $i $j ${NUMERI[y]} >> ${NOMIPERFILE[y]}
			riempifile
		done
		cd ..
	done
	cd ..
done
cd ..
echo file generati
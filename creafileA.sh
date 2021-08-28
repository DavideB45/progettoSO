#!/bin/bash

DIRS=(a b c d e)
NUMERI=(primo secondo terzo quarto quinto sesto)
NOMIPERFILE=(file1 file2 file3 file4 file5 file6)
rm -r "filePerTest"
mkdir "filePerTest"
cd "filePerTest"
function riempifile(){
	for lettera in {a..z};do
		if (($RANDOM % 4));then
			echo -n $lettera >> ${NOMIPERFILE[y]}
		fi
		if (($RANDOM % 4));then
			echo -n $lettera >> ${NOMIPERFILE[y]}
		fi
		if (($RANDOM % 4));then
			echo -n $lettera >> ${NOMIPERFILE[y]}
		fi
	done
	echo "newline" >> ${NOMIPERFILE[y]}
}
echo inizio generazione file
for (( i=0; i<${#DIRS[@]}; i++ )) do
mkdir ${DIRS[$i]}
cd ${DIRS[$i]}
	for (( j=0; j<${#DIRS[@]}; j++ )) do
		mkdir ${DIRS[$j]}
		cd ${DIRS[$j]}
			for (( y=0; y<${#NUMERI[@]}; y++)) do
				echo $i $j ${NUMERI[y]} > ${NOMIPERFILE[y]}
				for ((z=0; z<30; z++)) do
					riempifile
				done
			done
		cd ..
	done
cd ..
done
cd ..
echo file generati
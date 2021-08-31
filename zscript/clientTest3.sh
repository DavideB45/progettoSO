#!/bin/bash

# {
# 	sleep 30 
# 	kill -INT $(ps aux | grep server.out | tr -s " " | cut -d ' ' -f 2 | head -n 1)
# 	kill -INT $(ps aux | grep client.out | tr -s " " | cut -d ' ' -f 2 | head -n 1)
# 	echo ciao
# 	echo sono ancora vivo
# } &


SOCKET=(-f socket3)
DIR=(a b c d e)
FILEN=(file1 file2 file3 file4 file5 file6)
function randomfile(){
	RFILE[0]="$PWD/filePerTest/${DIR[($RANDOM % 5)]}/${DIR[($RANDOM % 5)]}/${FILEN[($RANDOM % 6)]}"
	RFILE[1]="$PWD/filePerTest/${DIR[($RANDOM % 5)]}/${DIR[($RANDOM % 5)]}/${FILEN[($RANDOM % 6)]}"
	RFILE[2]="$PWD/filePerTest/${DIR[($RANDOM % 5)]}/${DIR[($RANDOM % 5)]}/${FILEN[($RANDOM % 6)]}"
	RFILE[3]="$PWD/filePerTest/${DIR[($RANDOM % 5)]}/${DIR[($RANDOM % 5)]}/${FILEN[($RANDOM % 6)]}"
	RFILE[4]="$PWD/filePerTest/${DIR[($RANDOM % 5)]}/${DIR[($RANDOM % 5)]}/${FILEN[($RANDOM % 6)]}"
}
function randomdir(){
	RDIR[0]="$PWD/filePerTest/${DIR[($RANDOM % 5)]}/${DIR[($RANDOM % 5)]}"
	RDIR[1]="$PWD/filePerTest/${DIR[($RANDOM % 5)]}"
}
function randomreq(){
	randomfile
	randomdir
	OPwD=(-w ${RDIR[0]} -D "$PWD/expelled" -r "${RDIR[0]}/${FILEN[($RANDOM % 6)]}" -d "$PWD/expelled" -l ${RDIR[0]}/${FILEN[($RANDOM % 6)]})
	OPw2=(-w ${RDIR[1]} -r "${RDIR[1]}/b/a/file1" -d "$PWD/expelled" -l "${RDIR[1]}/e/e/file5" -c "${RDIR[1]}/e/e/file5")
	OPW1=(-W ${RFILE[0]} -r "${RFILE[0]}" -d "$PWD/expelled")
	OPW2=(-W "${RFILE[0]},${RFILE[1]},${RFILE[3]}" -l ${RFILE[0]} -c ${RFILE[0]})
	OPRd=(-R "n=$((($RANDOM % 6) + 1))" -d "$PWD/expelled")
	OPC1=(-l ${RFILE[3]} -c ${RFILE[3]})
	OPC2=(-W ${RFILE[4]} -l ${RFILE[4]} -c ${RFILE[4]})
	OPLU=(-W ${RFILE[2]} -l ${RFILE[2]} -u ${RFILE[2]})
	# ALLARR=(${OPwD[@]} ${OPw2[@]} ${OPW1[@]} ${OPW2[@]} ${OPRd[@]} ${OPC1[@]} ${OPC2[@]} ${OPLU[@]})
	PRIMA="$(($RANDOM % 8))"
	SECONDA="$((($PRIMA + ($RANDOM % 3) + 2) % 8))"
	TERZA="$((($SECONDA + ($RANDOM % 2) + 2) % 8))"
	ALLARR[0]=${OPwD[@]}
	ALLARR[1]=${OPw2[@]}
	ALLARR[2]=${OPW1[@]}
	ALLARR[3]=${OPW2[@]}
	ALLARR[4]=${OPRd[@]}
	ALLARR[5]=${OPC1[@]}
	ALLARR[6]=${OPC2[@]}
	ALLARR[7]=${OPLU[@]}
	# echo ${ALLARR[0]}
	# echo ${ALLARR[0,*]}
	RREQUEST=(${ALLARR[$PRIMA]} ${ALLARR[$SECONDA]} ${ALLARR[TERZA]})
}

OPER=("CLOSE")

RFILE=(array)
randomfile
RDIR=(array)
randomdir
RREQUEST=(array)
SECONDS=0
while (($SECONDS <= 30))
do
	if (( $(ps aux | grep ilClient.out | wc -l) < 17))
	then
		for ((i=0;i<25;i++)) do
			randomreq
			./bin/ilClient.out -f ./servWork/socket3 ${RREQUEST[@]} &
		done
	fi
done

kill -INT $(ps aux | grep -v grep | grep ./bin/server.out | tr -s " " | cut -d ' ' -f 2 | head -n 1)
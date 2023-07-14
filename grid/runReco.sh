#!/bin/bash
CTF_LIST=`alien_ls $1`

for CTF in $CTF_LIST
do
	CMD="alien_submit /alice/cern.ch/user/c/chizh/mch/commissioning/2023/reco.jdl 535047 LHC23e ${CTF:0:4} 7200"
	echo $CMD
	${CMD}
done

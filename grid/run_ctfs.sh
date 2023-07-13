#!/bin/bash
CTF_LIST=`alien_ls $TARGET_DIR`

for CTF in $CTF_LIST
do
	CMD="submit /alice/cern.ch/user/c/chizh/mch/commissioning/2023/reco.jdl 535047 LHC23e ${CTF:0:4} 10000"
	echo $CMD
	${CMD}
done

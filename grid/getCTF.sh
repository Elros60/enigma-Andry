#!/bin/bash
TARGET_DIR=$1
CTF_LIST=`alien_ls $TARGET_DIR`
for CTF in $CTF_LIST
do
	python3 getFileList.py --run 535047 --period LHC23e --ctf $CTF
done


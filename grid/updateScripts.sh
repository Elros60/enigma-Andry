#!/bin/bash
TARGET_DIR=$1

for FILE in reco.jdl reco.sh reco_validation.sh
do
	echo "Copying file: $FILE ..."
	alien_cp file://$FILE alien:///alice/cern.ch/user/c/chizh/mch/commissioning/2023/$FILE
done


#!/usr/bin/env bash

currentDir=`pwd`

echo "* *****************************************************"
echo "* Executing reco.sh ..."
echo "* O2PHYSICS_ROOT=${O2PHYSICS_ROOT}"
echo "* O2_ROOT=${O2_ROOT}"
echo "* Workdir: ${currentDir}"
echo "* *****************************************************"
echo ""

## Ensure necessary files are present in the node work directory

localCcdbBaseDir="${currentDir}"

geomFilenameAligned="o2sim_geometry_AlignFix1_6_060223_new.root"
if [ ! -e "${geomFilenameAligned}" ]; then
    echo "Cannot find $geomFilenameAligned, exit"
    exit 1
fi
ccdbGeomAlignedPath="GLO/Config/GeometryAligned"
mkdir -p "${localCcdbBaseDir}/${ccdbGeomAlignedPath}"
cp -pfv ${geomFilenameAligned} ${localCcdbBaseDir}/${ccdbGeomAlignedPath}/snapshot.root

if [ ! -e "${localCcdbBaseDir}/${ccdbGeomAlignedPath}/snapshot.root" ]; then
    echo "Cannot find ${localCcdbBaseDir}/${ccdbGeomAlignedPath}/snapshot.root, exit"
    exit 1
fi

#magFilename="o2sim_grpMagField.root"
#if [ ! -e "${magFilename}" ]; then
#    echo "Cannot find $magFilename, exit"
#    exit 1
#fi
#ccdbMagPath="GLO/Config/GRPMagField"
#mkdir -p "${localCcdbBaseDir}/${ccdbMagPath}"
#cp -pfv ${magFilename} ${localCcdbBaseDir}/${ccdbMagPath}/snapshot.root

#if [ ! -e "${localCcdbBaseDir}/${ccdbMagPath}/snapshot.root" ]; then
#    echo "Cannot find ${localCcdbBaseDir}/${ccdbMagPath}/snapshot.root, exit"
#    exit 1
#fi


inFilename="${1-wn.xml}"
tmpInFile="wn.txt"
sed -rn 's/.*turl="([^"]*)".*/\1/p' ${inFilename} > ${tmpInFile}

if [ ! -e "${tmpInFile}" ]; then
    echo "Cannot find ${tmpInFile}, exit"
    exit 1
fi

shmSize=16000000000

severity="info"
logConfig="--severity ${severity} --timeframes-rate-limit 3 --timeframes-rate-limit-ipcid 0 " #--infologger-mode \"stdout\""

#readCmd="o2-ctf-reader-workflow --copy-cmd no-copy --ctf-input ${tmpInFile} --delay 1.5 --loop 0 --onlyDet MCH,MID --shm-segment-size ${shmSize} ${logConfig} --allow-missing-detectors --condition-remap file://${localCcdbBaseDir}=${ccdbGeomAlignedPath},${ccdbMagfieldPath} -b "

readCmd="o2-ctf-reader-workflow --copy-cmd \"alien_cp ?src file://?dst\" --remote-regex \"^alien:///alice/data/.+\"  --ctf-input ${tmpInFile} --delay 1.5 --loop 0 --onlyDet MCH,MID --shm-segment-size ${shmSize} ${logConfig} --allow-missing-detectors --condition-remap file://${localCcdbBaseDir}=${ccdbGeomAlignedPath} -b "

recoOptions="MCHTracking.maxCandidates=20000;MCHTracking.maxTrackingDuration=10;;;MCHTracking.chamberResolutionX=0.4;MCHTracking.chamberResolutionY=0.4;MCHTracking.sigmaCutForTracking=7;MCHTracking.sigmaCutForImprovement=6;"

recoCmd="o2-mid-reco-workflow --shm-segment-size ${shmSize} ${logConfig} --disable-mc --pipeline MIDClusterizer:1,MIDTracker:1 | o2-mch-reco-workflow --shm-segment-size ${shmSize} ${logConfig} --disable-root-input --disable-mc --pipeline mch-track-finder:1,mch-cluster-finder:1,mch-cluster-transformer:1 --configKeyValues \""${recoOptions}"\" --condition-remap file://${localCcdbBaseDir}=${ccdbGeomAlignedPath} -b | o2-muon-tracks-matcher-workflow  --shm-segment-size ${shmSize} ${logConfig} --disable-root-input --disable-mc  --pipeline muon-track-matcher:1 "

## Concatenate workflow

runCmd=" $readCmd "
runCmd+=" | $recoCmd"
runCmd+=" | o2-dpl-run ${logConfig} --shm-segment-size ${shmSize} -b --run > reco.log"

## List input files and command line

echo ""
echo "======================="
echo "xml file list "${inFilename}
echo "First files listed within ${tmpInFile} :"
head -n 2  ${tmpInFile}
echo "Total number of files within ${tmpInFile} :"
cat  ${tmpInFile} | wc -l
echo "======================="
echo "Running reconstruction command: "
echo "${runCmd} "
echo "======================="
startTime=$(date +"%Y %m %d %H:%M:%S")
echo "Start "${startTime}
echo "======================="

## Runs reconstruction comand stored in $runCmd

echo | eval "${runCmd}" # "echo | " is a hack (to provide input stream to O2 workflows?)

endTime=$(date +"%Y %m %d %H:%M:%S")
echo "End "${endTime}
echo "======================="
echo ""

exit "$?"

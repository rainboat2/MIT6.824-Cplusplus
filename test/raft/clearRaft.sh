#!/bin/bash

logDir=../../logs
raftNum=3

rm -rf $logDir
mkdir $logDir

for i in $( seq 1 $raftNum)
do
    mkdir $logDir/raft$i
    lsof -i:800$i -t | xargs kill -9
done

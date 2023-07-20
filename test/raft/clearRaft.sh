#!/bin/bash

logDir=../../logs
raftNum=7

mkdir -p $logDir
rm -rf $logDir/*

for i in $( seq 1 $raftNum)
do
    lsof -i:700$i -t | xargs kill -9
done

mkdir -p $logDir/test_raft
rm -f $logDir/test_raft/
#!/bin/bash

logDir=../../logs
raftNum=7

mkdir -p $logDir

for i in $( seq 1 $raftNum)
do
    mkdir -p $logDir/raft$i
    rm -f $logDir/raft$i/*
    lsof -i:700$i -t | xargs kill -9
done

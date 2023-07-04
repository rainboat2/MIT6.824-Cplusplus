#!/bin/bash

workerNum=3
ep=(-1 0 0 0.8)
dp=(-1 0 0.5 0)

workDir=$(cd $(dirname $0); pwd)
master=$workDir/bin/master
worker=$workDir/bin/worker
dataDir=$workDir/data

cd $dataDir
mkdir -p $workDir/logs/master 
$master pg-*.txt &


# wait 0.5s for starting master
sleep 0.5


for i in $( seq 1 $workerNum )
do
    logDir=$workDir/logs/worker$i
    mkdir -p $logDir
    $worker --log_dir=$logDir --exit_possibility=${ep[$i]} --delay_possibility=${dp[$i]} &
done


echo "The calculation process is shown in the logs folder and the results in data/mr-out-*"
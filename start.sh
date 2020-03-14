#!/bin/bash

exePath=$(cd "$(dirname "$0")";pwd)
nowStr=`date +"%Y-%m-%d %H:%M:%S"`
echo ${nowStr} $exePath

IsRun=`ps -x |grep master_server | grep -v grep`
if [ "$?" != "0" ]; then
		echo "${nowStr}>>>> starting master_server"
    $exePath/bin/master_server &
fi

IsRun=`ps -x |grep master_ws | grep -v grep`
if [ "$?" != "0" ]; then
		echo "${nowStr}>>>> starting master_ws"
    $exePath/bin/master_ws &
fi

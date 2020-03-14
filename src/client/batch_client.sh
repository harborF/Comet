#!/bin/bash

exePath=$(cd "$(dirname "$0")"; pwd)
for((i=0;i<10;++i))
do
  nohup $exePath/TestClient -s '120.76.127.186:8888' -m 5 -f 100 &
done

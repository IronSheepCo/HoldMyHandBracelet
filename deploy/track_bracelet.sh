#!/bin/sh

cleanup () 
{ 
    screen -ls | grep Detached | cut -d. -f1 | awk '{print $1}' | xargs kill 
    ps -x | grep JLinkExe | cut -f1 | awk '{print $1}' | xargs kill
     exit   
}

trap cleanup SIGHUP SIGTERM SIGINT

./go.sh
sleep 1
screen -dmS "JLink" JLinkExe -device nRF51422_xxAC -speed 4000
sleep 1s
JLinkRTTClient | python analyse_location.py 

cleanup;

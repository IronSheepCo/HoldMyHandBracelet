#!/bin/sh

cleanup () 
{ 
    screen -ls | grep Detached | cut -d. -f1 | awk '{print $1}' | xargs kill 
    ps -x | grep JLinkExe | cut -f1 | awk '{print $1}' | xargs kill
     exit   
}

trap cleanup SIGHUP SIGTERM SIGINT

should_deploy=1

while getopts "f" opt; do
    case "$opt" in
        f) should_deploy=0
            ;;
    esac
done

if [ $should_deploy -eq 1 ] 
    then ./go.sh
    if [ $? -ne 0 ]
        then cleanup;
    fi
fi

sleep 1
screen -dmS "JLink" JLinkExe -device nRF51422_xxAC -speed 4000
sleep 1s
shift
JLinkRTTClient | python analyse_location.py "$@" 

cleanup;

#!/usr/bin/env bash

while true
do
    ps aux | grep read.py | grep -v grep > /dev/null 
    if [ $? -ne 0 ]
    then
      echo `date` ": DEAD!"
      echo `date` ": respawning..."
      tmux send-keys -t app001:0.0 'workon serial' C-m
      tmux send-keys -t app001:0.0 'pushd /home/pi/readserial' C-m
      tmux send-keys -t app001:0.0 'python read.py' C-m
    else
      echo "ok"
    fi 
    sleep 10s
done

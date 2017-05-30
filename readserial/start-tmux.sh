#!/bin/bash

SHELL=/bin/bash
source /home/pi/.profile

workon serial

tmux new-session -s app001 -d

tmux split-window -h -p 45 -t app001
tmux select-pane -L
tmux split-window -v -p 30 -t app001

#tmux send-keys -t app001:0.0 'workon serial' C-m
#tmux send-keys -t app001:0.0 'pushd /home/pi/readserial' C-m
#tmux send-keys -t app001:0.0 'python read.py' C-m
tmux send-keys -t app001:0.1 'cd ~' C-m
tmux send-keys -t app001:0.2 'pushd /home/pi/readserial; . ./god.sh' C-m

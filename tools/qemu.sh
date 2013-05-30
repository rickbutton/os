#!/bin/bash
tmux split-window -h 'qemu-system-i386 -m 32 -kernel build/kernel/kernel.elf -curses -monitor telnet:localhost:4444,server -s'
tmux select-pane -L
sleep 0.1
telnet localhost 4444

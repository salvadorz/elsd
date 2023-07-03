#!/bin/bash
# Author: Salvador Zendejas


set -e
# set -u

function pause() {
    read -t 10 -s -n 1 -p "$*"
    echo ""
}

# Process by default
PROCESS="aesdsocket"

# Exits with return value 1 error and print statements if any of the parameters above were not specified
if [ $# -gt 0 ]; then
    PROCESS=$1
fi

ppid=$(pgrep "$PROCESS")

if [ -n ${ppid} ]; then
    echo "Process $PROCESS has PID: $ppid"
else
    echo "Process $PROCESS is not running"
    exit 0
fi

pause "Do you want to kill the process? (Y/N) "
if [[ "$REPLY" == "Y" || "$REPLY" == "y" ]]; then
    echo "Killing $ppid"
    kill -s SIGKILL "$ppid"
else
    echo "Bye..."
fi
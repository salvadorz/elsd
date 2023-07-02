#!/bin/sh
# Author: Salvador Zendejas


set -e
set -u

# Exits with return value 1 error and print statements if any of the parameters above were not specified
usage() { echo "    Usage: $0 [PROGRAM_BINARY]" 1>&2; exit 1; }

# Accepts the following runtime arguments: the first argument is a path to a directory on the filesystem, referred to below as filesdir;
if [ $1 ] && [ -n $1 ]
then
    progname=$1
else
    echo "-First parameter needs to be a program binary."
fi

if [ $# -lt 1 ]
then
    usage
fi

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=./valgring_report.txt ${progname}
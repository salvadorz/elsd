#!/bin/sh
# Finder script for assignment 1
# Author: Salvador Zendejas


set -e
set -u

# Exits with return value 1 error and print statements if any of the parameters above were not specified
usage() { echo "    Usage: $0 [SEARCH/DIR] [SEARCH_STRING]" 1>&2; exit 1; }

# Accepts the following runtime arguments: the first argument is a path to a directory on the filesystem, referred to below as filesdir;
if [ $1 ] && [ -d $1 ]
then
    filesdir=$1
else
    echo "-First parameter needs to be a directory where to search within these files."
    fi
# the second argument is a text string which will be searched within these files, referred to below as searchstr
if [ $2 ] && [ -n $2 ]
then
    searchstr=$2
else
    echo "-Second parameter needs to be a Search string."
fi

if [ $# -lt 2 ]
then
    usage
fi

files=$(find $filesdir -type f 2>/dev/null | wc -l)
lines=$(grep -snioR $searchstr $filesdir | wc -l)
echo "The number of files are ${files} and the number of matching lines are ${lines}"

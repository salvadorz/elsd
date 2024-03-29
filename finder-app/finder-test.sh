#!/bin/sh
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

set -e
set -u

HW=`uname -i`
if [[ $HW == x86* ]]; then
CONF="conf"
else
CONF="/etc/finder-app/conf"
fi

NUMFILES=10
WRITER=writer.sh
WRITESTR=AELD_IS_FUN
WRITEDIR=/tmp/aeld-data
username=$(cat ${CONF}/username.txt)
dirpath=$(dirname $0)
# write a file with output of the finder command to
output_file=/tmp/assignment4-result.txt

if [ $# -lt 3 ]
then
	echo "Using default value ${WRITESTR} for string to write"
	if [ $# -lt 1 ]
	then
		echo "Using default value ${NUMFILES} for number of files to write"
	else
		NUMFILES=$1
	fi	
else
	NUMFILES=$1
	WRITESTR=$2
	WRITEDIR=/tmp/aeld-data/$3
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${WRITEDIR}"

rm -rf "${WRITEDIR}"

# create $WRITEDIR if not assignment1
assignment=`cat ${CONF}/assignment.txt`

if [ $assignment != 'assignment1' ]
then
	mkdir -p "$WRITEDIR"
	WRITER=writer

	#The WRITEDIR is in quotes because if the directory path consists of spaces, then variable substitution will consider it as multiple argument.
	#The quotes signify that the entire string in WRITEDIR is a single string.
	#This issue can also be resolved by using double square brackets i.e [[ ]] instead of using quotes.
	if [ -d "$WRITEDIR" ]
	then
		echo "$WRITEDIR created"
	else
		exit 1
	fi
fi
if [ $assignment == 'assignment2' ]
then
    #echo "Removing the old writer utility and compiling as a native application"
    make clean
    make
fi

for i in $( seq 1 $NUMFILES)
do
	${dirpath}/${WRITER} "$WRITEDIR/${username}$i.txt" "$WRITESTR"
done

OUTPUTSTRING=$(${dirpath}/finder.sh "$WRITEDIR" "$WRITESTR")

# remove temporary directories
rm -rf /tmp/aeld-data

set +e
echo ${OUTPUTSTRING} | grep "${MATCHSTR}" >> ${output_file}
if [ $? -eq 0 ]; then
	echo "success"
	exit 0
else
	echo "failed: expected  ${MATCHSTR} in ${OUTPUTSTRING} but instead found"
	exit 1
fi

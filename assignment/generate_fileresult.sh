#!/bin/bash
# Generates cross-compile.txt script for assignment 2
# Author: Salvador Zendejas

set -e

FILE=writer
OUTPUT="../assignment/assignment2/fileresult.txt"

cd ../finder-app
echo -e "Printing file info, for ${FILE}\n"
file ${FILE} > ${OUTPUT} 2>&1
cat ${OUTPUT}
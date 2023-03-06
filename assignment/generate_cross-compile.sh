#!/bin/bash
# Generates cross-compile.txt script for assignment 2
# Author: Salvador Zendejas

echo -e "Printing version, configuration and sysroot path in cross-compile.txt...\n"
aarch64-none-linux-gnu-gcc -print-sysroot -v > assignment2/cross-compile.txt 2>&1
cat assignment2/cross-compile.txt
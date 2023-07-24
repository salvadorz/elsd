#! /bin/sh
# @author: Salvador Z.

module=aesdchar

cd `dirname $0`
set -e

src_file="./$module.ko"
dst_file="/lib/modules/$(uname -r)/$module.ko"

if [ ! -e $src_file ]; then
    make
    if [ $? -ne 0 ]; then
        echo "Error in make"
        exit 1
    fi
fi

# Verificar si el archivo de origen es más nuevo que el destino
if [ "$src_file" -nt "$dst_file" ]; then
    sudo rsync -av "$src_file" "$dst_file"
    echo "$module updated to $dst_file"
fi

sudo ../aesd-char-driver/aesdchar_unload 
sudo ../aesd-char-driver/aesdchar_load
strace -o  ../aesd-char-driver/strace.log -f ../assignment-autotest/test/assignment8/drivertest.sh

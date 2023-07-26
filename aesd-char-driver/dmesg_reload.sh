#! /bin/sh

entries=5
tmp_file="tmp_dmesg"
tmp_file_cl="tmp_dmesg_cleaned"

cd `dirname $0`
set -e

sudo dmesg > $tmp_file

if [ $# -gt 0 ]; then
  # Utilizar el valor del argumento como el número de líneas a eliminar
  entries=$1
fi

# Obtener las últimas líneas de dmesg_temp
tail -n $entries $tmp_file > log.txt

# Eliminar las últimas líneas de dmesg_temp
head -n -"$entries" $tmp_file > ${tmp_file_cl}

# Borrar el registro actual de dmesg
dmesg -C

# Cargar el archivo nuevamente en el registro del kernel
dmesg -r < ${tmp_file_cl}

# Eliminar el archivo temporal
rm ${tmp_file} ${tmp_file_cl}
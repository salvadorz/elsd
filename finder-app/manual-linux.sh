#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo & Salvador Z.

set -e
set -u

function pause(){
    read -t 10 -s -n 1 -p "$*"
    echo ""
}

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_TARB=https://mirrors.edge.kernel.org/pub/linux/kernel
KERNEL_VERSION=v5.1.10
KERNEL_MAJOR=${KERNEL_VERSION:0:2}
KERNEL_NUMBER=${KERNEL_VERSION:1:6}
KERNEL_IMGTAR=${KERNEL_TARB}/${KERNEL_MAJOR}.x/linux-${KERNEL_NUMBER}.tar.gz
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
ARCH_SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
CORES=$(nproc)

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

if [ ! -d "${OUTDIR}" ]; then
    mkdir -p ${OUTDIR}
    if [ $? -ne 0 ]; then
        echo "Failed to create directory ${OUTDIR}"
        exit 1
    fi
fi

cd "$OUTDIR"
# TODO CHECK SOME FLAG/ARG WHETER TO CLONE OR DOWNLOAD
# if flag then curl -o linux-${KERNEL_NUMBER}.tar.gz $KERNEL_IMGTAR
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    #deep clean kernel build tree
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE mrproper
    #fix multiple definition of 'yylloc' 
    sed -i 's/^YYLTYPE /extern YYLTYPE /g' ${OUTDIR}/linux-stable/scripts/dtc/dtc-lexer.l
    #Config our "virt" ARCH dev board
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
    # Build a kernel image for booting with QEMU
    make -j${CORES} ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE all
    make -j${CORES} ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE modules
    make -j${CORES} ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE dtbs
fi

echo -n "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image $OUTDIR
if [ $? -eq 0 ]; then
    echo "...[OK]"
fi

# pause "Generating rootfs. Press [Enter] key to continue..."

cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm -rf ${OUTDIR}/rootfs
fi

if [ ! -d "${OUTDIR}/rootfs" ]; then
    echo -n "Creating the staging directory for the root filesystem"
    mkdir -p ${OUTDIR}/rootfs
    cd ${OUTDIR}/rootfs
    mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr
    mkdir -p usr/bin usr/lib usr/sbin
    mkdir -p var/log
    echo "...[OK]"
fi

# pause "Busybox Utility. Press [Enter] key to continue..."

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    echo "Configuring busybox"
    make distclean
    make defconfig
else
    cd busybox
fi

# pause "Make and Install Busybox. Press [Enter] key to continue..."
if [ ! -e ${OUTDIR}/rootfs/bin/busybox ]; then
    make -j8 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE CONFIG_PREFIX=$OUTDIR/rootfs install
fi

echo "Library dependencies"
${CROSS_COMPILE}readelf -a $OUTDIR/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a $OUTDIR/rootfs/bin/busybox | grep "Shared library"

# pause "Checking library dependencies to rootfs. Press [Enter] key to continue..."
#program interpreter ld
echo -n "Adding Library dependencies to rootfs"
if [ ! -e ${OUTDIR}/rootfs/lib64/ld-2.31.so ]; then
    cp -a $ARCH_SYSROOT/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
    cp -a $ARCH_SYSROOT/lib64/ld-2.31.so ${OUTDIR}/rootfs/lib64
fi
#libm
if [ ! -e ${OUTDIR}/rootfs/lib64/libm-2.31.so ]; then
    cp -a $ARCH_SYSROOT/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64
    cp -a $ARCH_SYSROOT/lib64/libm-2.31.so ${OUTDIR}/rootfs/lib64
fi
#libresolv
if [ ! -e ${OUTDIR}/rootfs/lib64/libresolv-2.31.so ]; then
    cp -a $ARCH_SYSROOT/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64
    cp -a $ARCH_SYSROOT/lib64/libresolv-2.31.so ${OUTDIR}/rootfs/lib64
fi
#libc
if [ ! -e ${OUTDIR}/rootfs/lib64/libc-2.31.so ]; then
    cp -a $ARCH_SYSROOT/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64
    cp -a $ARCH_SYSROOT/lib64/libc-2.31.so ${OUTDIR}/rootfs/lib64
fi
echo "...[OK]"

# pause "Make Devices Nodes. Press [Enter] key to continue..."
if [ ! -e ${OUTDIR}/rootfs/dev/null ]; then
    echo "Make device nodes..."
    sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
    sudo mknod -m 600 ${OUTDIR}/rootfs/dev/console c 5 1
fi

# pause "Build Writer Utility. Press [Enter] key to continue..."
echo "Clean and build the app(s) utility from ${FINDER_APP_DIR}"
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}


echo -n "Copy related scripts to /home on target rootfs"
cp -r ${FINDER_APP_DIR}/*.sh ${OUTDIR}/rootfs/home/.
cp  ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home/.
cp -rL ${FINDER_APP_DIR}/conf ${OUTDIR}/rootfs/home/.
echo "...[OK]"

echo -n "Chown the root directory"
cd "${OUTDIR}/rootfs"
sudo chown -R root:root *
echo "...[OK]"

echo "Create initramfs.cpio.gz"
cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd "${OUTDIR}"
gzip -f initramfs.cpio
if [ -e "${OUTDIR}/initramfs.cpio.gz" ]; then
    echo "==Disk Image ready to be loaded in RAM. Start QEMU!!=="
else
    echo "Error. Disk Image not ready to be loaded in RAM."
    exit 1
fi

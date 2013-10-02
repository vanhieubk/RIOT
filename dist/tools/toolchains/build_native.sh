#!/bin/bash

# directory to install compiled binaries into
PREFIX=${HOME}/native-toolchain

# directory to download source files and store intermediates 
TMP_DIR=/var/tmp
TOOLCHAIN_BUILDDIR=${TOOLCHAIN_BUILDDIR:-"${TMP_DIR}/native-${USER}"}

NEWLIB_VER=1.20.0
NEWLIB_MD5=e5488f545c46287d360e68a801d470e8

#uncomment to support multi-threaded compile
MAKE_THREADS=-j4

DOWNLOADER=wget
DOWNLOADER_OPTS="-nv -c"

#
# Build targets
#
FILES=.

HOST_GCC_VER=`gcc --version | awk '/gcc/{print $NF}'`

SPACE_NEEDED=2641052
FREETMP=`df ${TMP_DIR} | awk '{ if (NR == 2) print $4}'`


extract_newlib() {
    if [ ! -e .newlib_extracted ] ; then
        echo -n "Extracting newlib..."
        tar -xzf ${FILES}/newlib-${NEWLIB_VER}.tar.gz &&
        touch .newlib_extracted &&
        echo " Done."
    fi
}

build_newlib() {
	cd ${TOOLCHAIN_BUILDDIR} &&
    
    if [ ! -e .newlib_extracted ] ; then 
        extract_newlib
    fi 
	
    rm -rf newlib-build && mkdir -p newlib-build && cd newlib-build &&
	CFLAGS=-m32 ../newlib-${NEWLIB_VER}/configure --with-newlib --host=i686-none --prefix=${PREFIX} --disable-newlib-supplied-syscalls &&
    #--enable-interwork --enable-multilib --disable-newlib-supplied-syscalls --enable-newlib-reent-small --enable-newlib-io-long-long --enable-newlib-io-float 
	#--enable-newlib-supplied-syscalls &&
	# options to try: --enable-newlib-reent-small
    make ${MAKE_THREADS} TARGET_CFLAGS=-DREENTRANT_SYSCALLS_PROVIDED all &&
    make install &&
    
    cd ${TOOLCHAIN_BUILDDIR}
}

clean() {
    echo "Cleaning up..."
    rm -rf .newlib_extracted
    rm -rf newlib-build
}

export PATH=$PATH:${PREFIX}/bin

download() {
    download_file ftp://sources.redhat.com/pub/newlib newlib-${NEWLIB_VER}.tar.gz ${NEWLIB_MD5}
}

download_file() {
    echo "Downloading ${1}/${2}..."
    ${DOWNLOADER} ${DOWNLOADER_OPTS} $1/$2

    echo -n "Checking MD5 of "
    echo "${3}  ${2}" | md5sum -c -
}

check_space() {
    echo "Checking disk space in ${TMP_DIR}"
    if [ $FREETMP -lt $SPACE_NEEDED ]
    then
        echo "Not enough available space in ${TMP_DIR}. Minimum ${SPACE_NEEDED} free bytes required."
        exit 1
    fi
}

build() {
    echo "Starting in ${TOOLCHAIN_BUILDDIR}. Installing to ${PREFIX}."
    check_space &&
    download &&
    extract_newlib &&
	build_newlib &&

	echo "Build complete."
}

usage() {
    echo "usage: ${0} build"
    echo "example: ${0} build"
    echo ""
    echo "Builds a newlib. installs to ${PREFIX}, uses ${TOOLCHAIN_BUILDDIR} as temp."
    echo "Edit to change these directories."
    echo "Run like \"MAKE_THREADS=-j4 ${0} build\" to speed up on multicore systems."
}

if [ -z "${1}" ]; then
    usage
    exit 1
fi

mkdir -p ${TOOLCHAIN_BUILDDIR}

cd ${TOOLCHAIN_BUILDDIR}

$*


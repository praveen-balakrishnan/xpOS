#!/bin/bash

BINUTILS_VERSION=$1

export PREFIX="/usr/local/cross"
export TARGET=x86_64-xpos
export PATH="$PREFIX/bin:$PATH"
export SYSROOT="$HOME/env/SystemRoot"

cd $PREFIX/src
mkdir build-binutils
cd build-binutils
../binutils-${BINUTILS_VERSION}/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot="$SYSROOT" --disable-nls --disable-werror --enable-shared
make -j 4
make install

cd $PREFIX/src
rm -rf build-binutils.sh build-binutils binutils-${BINUTILS_VERSION}
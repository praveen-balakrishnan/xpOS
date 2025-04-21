#!/bin/bash

GCC_VERSION=$1

export PREFIX="/usr/local"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

cd $PREFIX/src/gcc-${GCC_VERSION}/gcc
patch < config.gcc.patch

cd $PREFIX/src
mkdir build-gcc
cd build-gcc
../gcc-${GCC_VERSION}/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --without-cloog --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
make -j 4 all-gcc
make -j 4 all-target-libgcc
make -j 4 all-target-libstdc++-v3
make -j 4 install-gcc
make -j 4 install-target-libgcc
make -j 4 install-target-libstdc++-v3

cd $PREFIX/src
# xpOS

An x86-64 hobbyist operating system written from scratch in C++.

## Features

* A 64-bit kernel written using C++
* Pre-emptive multitasking with a priority-based scheduler
* ELF loader and custom toolchain support
* A modern graphical user interface stack
* Network stack (IPv4, TCP, etc.) and sockets support
* [mlibc](https://github.com/managarm/mlibc) used as a C library

## Screenshots

![xpOSScreenshot](https://github.com/user-attachments/assets/71e47fec-c0ae-4b46-9f89-904aef416e42)
<img width="1278" alt="xpOSScreenshot2" src="https://github.com/user-attachments/assets/aad94eb3-928d-4974-b2d4-b9b0fceed3e6" />


## Building and Running

The recommended way of building xpOS is to use Docker. The provided Docker image will install dependencies and build the GNU toolchain.

```
docker build buildenv -t xpos-buildenv
```

Then, generating an ISO file is simple.

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="../Toolchain/GNUToolchain.cmake" -S..
make
```

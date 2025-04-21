#
#	Copyright 2023-2025 Praveen Balakrishnan
# 
#	Licensed under the Apache License, Version 2.0 (the "License");
#	you may not use this file except in compliance with the License.
#	You may obtain a copy of the License at
#
#     	http://www.apache.org/licenses/LICENSE-2.0
# 
# 	Unless required by applicable law or agreed to in writing, software
# 	distributed under the License is distributed on an "AS IS" BASIS,
# 	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# 	See the License for the specific language governing permissions and
# 	limitations under the License.
#
# 	xpOS v1.0

FROM ubuntu:22.04

ARG BINUTILS_VERSION=2.41
ARG GCC_VERSION=13.2.0

# Install cross-compiler prerequisites
RUN set -x \
	&& apt-get update \
	&& apt-get install -y cmake wget gcc libgmp3-dev libmpfr-dev libisl-dev \
		libmpc-dev texinfo bison flex make bzip2 patch \
		build-essential python3 python3-pip ninja-build

# Pull binutils and gcc source code
RUN set -x \
	&& mkdir -p /usr/local/src \
	&& cd /usr/local/src \
	&& wget -q https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.gz \
	&& wget -q https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz \
	&& tar zxf binutils-${BINUTILS_VERSION}.tar.gz \
	&& tar zxf gcc-${GCC_VERSION}.tar.gz \
	&& rm binutils-${BINUTILS_VERSION}.tar.gz gcc-${GCC_VERSION}.tar.gz \
	&& chown -R root:root binutils-${BINUTILS_VERSION} \
	&& chown -R root:root gcc-${GCC_VERSION} \
	&& chmod -R o-w,g+w binutils-${BINUTILS_VERSION} \
	&& chmod -R o-w,g+w gcc-${GCC_VERSION}

# Copy compile scripts
COPY files/src /usr/local/src/

# Copy gcc patches
COPY files/gcc/t-x86_64-elf /usr/local/src/gcc-${GCC_VERSION}/gcc/config/i386/
COPY files/gcc/config.gcc.patch /usr/local/src/gcc-${GCC_VERSION}/gcc/

# Build and install binutils and the cross-compiler
RUN set -x \
	&& cd /usr/local/src \
	&& ./build-binutils.sh ${BINUTILS_VERSION} \
	&& ./build-gcc.sh ${GCC_VERSION}

CMD ["/bin/bash"]

RUN apt-get update 
RUN apt-get upgrade -y
RUN apt-get install -y nasm
RUN apt-get install -y xorriso
RUN apt-get install -y grub-pc-bin
RUN apt-get install -y grub-common
RUN apt-get install -y ninja-build

RUN pip3 install meson

VOLUME /root/env
WORKDIR /root/env
cmake_minimum_required(VERSION 3.2)

project(xpOS
        VERSION 1.0
        DESCRIPTION "xpOS by Praveen Balakrishnan."
        LANGUAGES C CXX ASM)
enable_language(ASM_NASM)
set (CMAKE_CXX_STANDARD 23)

add_subdirectory(Kernel)
add_subdirectory(Userspace)

set(KernelBinary "${CMAKE_SOURCE_DIR}/Targets/x86_64/xpOS.iso")
set(targetISO "${CMAKE_SOURCE_DIR}/Targets/x86_64/xpOS.iso")

set(ProgramsToBuild

)

add_custom_command(
        OUTPUT ${targetISO}
        DEPENDS Kernel WindowServer ${ProgramsToBuild}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND mkdir -p Targets/x86_64/xpinitrd/System/Resources && cp -r SystemRoot/System/Resources targets/x86_64/xpinitrd/System
        COMMAND tar -cvf Targets/x86_64/iso/boot/xpinitrd.tar -C Targets/x86_64 xpinitrd
        COMMAND grub-mkrescue /usr/lib/grub/i386-pc -o Targets/x86_64/xpOS.iso targets/x86_64/iso
)

add_custom_target(CreateISO ALL DEPENDS ${targetISO})

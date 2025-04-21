set(X86_64_CPP_SOURCES
    print.cpp
)

set(X86_64_ASM_SOURCES
    boot/header.asm
    boot/main.asm
    boot/main64.asm
    interrupts/interrupts.asm
    task/task.asm
    userspace/userspace.asm
)
add_library(x86_64lib OBJECT ${X86_64_CPP_SOURCES})
add_library(x86_64asmlib OBJECT ${X86_64_ASM_SOURCES})
target_include_directories(x86_64lib PRIVATE ../../include/kernel ../../include/x86_64)
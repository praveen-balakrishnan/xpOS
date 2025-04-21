set(KERNEL_SOURCES
    Arch/CPU.cpp
    Arch/GDT.cpp
    Arch/Interrupts/Interrupts.cpp
    Arch/Interrupts/PIC.cpp
    Arch/IO/PCI.cpp
    Arch/IO/PIT.cpp
    Boot/Modules/ModuleManager.cpp
    Boot/Multiboot.cpp
    Common/Hash/MurmurHash3.cpp
    Common/String.cpp
    Drivers/HID/Keyboard.cpp
    Drivers/HID/Mouse.cpp
    Drivers/Initrd/TARinitrd.cpp
    Drivers/Network/PCNet.cpp
    Drivers/Network/NetworkDriver.cpp
    Drivers/Graphics/VMWare/SVGAII.cpp
    Filesystem/VFS.cpp
    guard-abi.cpp
    icxxabi.cpp
    kernel.cpp
    Loader/ELF.cpp
    Memory/AddressSpace.cpp
    Memory/dlmalloc.c
    Memory/dlmallochook.cpp
    Memory/KernelHeap.cpp
    Memory/Memory.cpp
    memory/MemoryManager.cpp
    memory/SharedMemory.cpp
    Networking/NetworkServer.cpp
    Networking/NetworkSocket.cpp
    Networking/Ethernet/Ethernet.cpp
    Networking/ARP/ARP.cpp
    Networking/IP/IPv4.cpp
    Networking/TCP/TCP.cpp
    Pipes/LocalSocket.cpp
    Pipes/EventListener.cpp
    Pipes/Pipe.cpp
    Tasks/Mutex.cpp
    Tasks/TaskManager.cpp
    Tasks/Spinlock.cpp
    Tasks/MasterScheduler.cpp
    Tasks/WaitQueue.cpp
    Syscalls/SyscallHandler.cpp
)
set(X86_64_SOURCES
    x86_64/Boot/header.asm
    x86_64/Boot/main.asm
    x86_64/Boot/main64.asm
    x86_64/Interrupts/interrupts.asm
    x86_64/Task/task.asm
    x86_64/Userspace/userspace.asm
    x86_64/print.cpp
)

set(SOURCES
    ${KERNEL_SOURCES}
    ${X86_64_SOURCES}
)

add_executable(Kernel ${SOURCES})
target_include_directories(Kernel PUBLIC ${CMAKE_CURRENT_LIST_DIR} x86_64)

target_compile_options(Kernel PRIVATE 
    $<$<COMPILE_LANGUAGE:CXX>:
        -Wall -Wextra -Wpedantic -Werror
        -fno-strict-aliasing
        -Wno-missing-field-initializers -Wno-address-of-packed-member -Wno-unused-function -Wno-unused-parameter 
        -g -mno-red-zone -nostartfiles -nostdlib  -fno-sized-deallocation -fno-rtti -fno-pic -fno-exceptions
        -ffreestanding -mcmodel=large -mno-80387 -mno-mmx -mno-sse -mno-sse2 -fno-asynchronous-unwind-tables -fno-stack-protector
        -fno-sanitize=all -ftrivial-auto-var-init=uninitialized -fno-tree-loop-distribution -fno-tree-loop-distribute-patterns
        -fno-tree-vectorize -lgcc
    >

    $<$<COMPILE_LANGUAGE:C>:
        -Wall -Wextra -Wpedantic -Werror 
        -fno-strict-aliasing
        -Wno-missing-field-initializers -Wno-address-of-packed-member -Wno-unused-function -Wno-unused-parameter 
        -g -mno-red-zone -nostartfiles -nostdlib -fno-pic -fno-exceptions -ffreestanding -mcmodel=large -mno-80387
        -mno-mmx -mno-sse -mno-sse2 -fno-asynchronous-unwind-tables -fno-stack-protector -fno-sanitize=all
        -ftrivial-auto-var-init=uninitialized -fno-tree-loop-distribution -fno-tree-loop-distribute-patterns
        -fno-tree-vectorize -lgcc
    >
)


target_link_options(Kernel PRIVATE LINKER:-T /root/env/targets/x86_64/linker.ld -nostdlib -nodefaultlibs --no-pie)

set_target_properties(
    Kernel 
    PROPERTIES 
        OUTPUT_NAME "xpOS"
        SUFFIX ".bin"
)
set_target_properties(Kernel PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/targets/x86_64/iso/boot")

#include "Pipe.h"
#include <API/Syscall.h>

namespace xpOS::OSLib
{
    using namespace xpOS::API;

    int popen(const char* device, void* with, int flags)
    {
        return Syscalls::syscall(
            SYSCALL_POPEN,
            reinterpret_cast<uint64_t>(device),
            reinterpret_cast<uint64_t>(with),
            flags
        );
    }

    bool pclose(uint64_t pd)
    {
        return Syscalls::syscall(
            SYSCALL_PCLOSE,
            pd
        );
    }

    std::size_t pread(uint64_t pd, void* buf, std::size_t count)
    {
        return Syscalls::syscall(
            SYSCALL_PREAD,
            pd,
            reinterpret_cast<uint64_t>(buf),
            count
        );
    }

    std::size_t pwrite(uint64_t pd, const void* buf, std::size_t count)
    {
        return Syscalls::syscall(
            SYSCALL_PWRITE,
            pd,
            reinterpret_cast<uint64_t>(buf), 
            count
        );
    }

    std::size_t pseek(uint64_t pd, long count)
    {
        return Syscalls::syscall(
            SYSCALL_PSEEK,
            pd,
            count
        );
    }
}
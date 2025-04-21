#include "Socket.h"

namespace xpOS::OSLib::LocalSocket
{
    bool connect(uint64_t pd, const char* id)
    {
        return xpOS::API::Syscalls::syscall(
            SYSCALL_LSOCK_CONNECT,
            pd,
            reinterpret_cast<uint64_t>(id)
        );
    }

    bool bind(uint64_t pd, const char* id)
    {
        return xpOS::API::Syscalls::syscall(
            SYSCALL_LSOCK_BIND,
            pd,
            reinterpret_cast<uint64_t>(id)
        );
    }

    bool listen(uint64_t pd)
    {
        return xpOS::API::Syscalls::syscall(
            SYSCALL_LSOCK_LISTEN,
            pd
        );
    }

    uint64_t accept(uint64_t pd)
    {
        return xpOS::API::Syscalls::syscall(
            SYSCALL_LSOCK_ACCEPT,
            pd
        );
    }
}
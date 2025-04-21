#ifndef XPOSLIB_SOCKET_H
#define XPOSLIB_SOCKET_H

#include "API/Syscall.h"
#include "API/Network.h"

namespace xpOS::OSLib::LocalSocket
{
    bool connect(uint64_t pd, const char* id);
    bool bind(uint64_t pd, const char* id);
    bool listen(uint64_t pd);
    uint64_t accept(uint64_t pd);

    enum class Flags : int
    {
        NON_BLOCKING = 1
    };

    enum class EventTypes : int
    {
        SOCK_REQUEST_CONN = 0x4
    };
}

#endif
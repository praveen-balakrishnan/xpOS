#ifndef XPOSLIB_PIPE_H
#define XPOSLIB_PIPE_H

#include <cstdint>

namespace xpOS::OSLib
{
    int popen(const char* device, void* with = nullptr, int flags = 0);
    bool pclose(uint64_t pd);
    std::size_t pread(uint64_t pd, void* buf, std::size_t count);
    std::size_t pwrite(uint64_t pd, const void* buf, std::size_t count);
    std::size_t pseek(uint64_t pd, long count);
}

#endif
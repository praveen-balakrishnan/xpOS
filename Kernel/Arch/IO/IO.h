/**
    Copyright 2023-2025 Praveen Balakrishnan

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    xpOS v1.0
*/

#ifndef IO_H
#define IO_H

#include <cstdint>

namespace IO
{
    
inline uint8_t in_8(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline uint16_t in_16(uint16_t port)
{
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline uint32_t in_32(uint16_t port)
{
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void out_8(uint16_t port, uint8_t value)
{
    asm volatile ("outb %0, %1" : : "a"(value), "Nd" (port)); 
}

inline void out_16(uint16_t port, uint16_t value)
{
    asm volatile ("outw %0, %1" : : "a"(value), "Nd" (port));
}

inline void out_32(uint16_t port, uint32_t value)
{
    asm volatile ("outl %0, %1" : : "a"(value), "Nd" (port));
}

inline void wait()
{
    out_8(0, 0x80);
}

}

#endif
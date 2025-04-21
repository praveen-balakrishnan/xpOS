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

#ifndef XPOS_MEMORY_H
#define XPOS_MEMORY_H

#include <cstdint>
#include <cstddef>

#define BYTE_ALIGN_UP(x,align)             __ALIGN_UP_MASK(x,(typeof(x))(align)-1)
#define __ALIGN_UP_MASK(x,mask)            (((x)+(mask))&~(mask))

#define BYTE_ALIGN_DOWN(x,align)             __ALIGN_DOWN_MASK(x,(typeof(x))(align)-1)
#define __ALIGN_DOWN_MASK(x,mask)            (((x))&~(mask))

#define CHECK_ALIGN(x,align)            !((x)&((typeof(x))(align)-1))

extern "C" inline void* default_memset(void* str, uint64_t c, uint64_t sz)
{
    uint8_t* p = (uint8_t*) str;
    while (sz--)
        *p++ = c;
    return str;
}

extern "C" inline void* fast_aligned_wordwise_64_memset(void* str, uint64_t c, uint64_t sz)
{
    uint64_t* p = (uint64_t*)str;
    c&=0xff;
    c|= c<<8;
    c|= c<<16;
    c|= c<<32;
    sz>>=3;
    while (sz--)
        *p++ = c;
    return str;
}

extern "C" inline void* fast_unaligned_wordwise_64_memset(void* str, uint64_t c, uint64_t sz)
{
    uint64_t* p;
    uint8_t* bp = (uint8_t*)str;
    uint8_t cp = c & 0xff;
    while (((uint64_t)bp&0b111)&&sz--)
        *bp++ = cp;
    p = (uint64_t*)bp;
    uint64_t t = sz&0b111;

    c|= c<<8;
    c|= c<<16;
    c|= c<<32;
    sz>>=3;
    while (sz--)
        *p++ = c;
    bp = (uint8_t*)p;
    while (t--)
        *bp++ = cp;
    return str;
}

extern "C" inline void* slow_memcpy(void* dst, const void* src, uint64_t sz)
{
    uint8_t* dptr = (uint8_t*)dst;
    uint8_t* sptr = (uint8_t*)src;
    while(sz--)
        *dptr++ = *sptr++;
    return dst;
}

extern "C" inline void* memset(void* str, uint64_t c, uint64_t sz)
{
    if (sz<16)
        return default_memset(str, c, sz);
    if ((uint64_t)str&0b111)
        return fast_unaligned_wordwise_64_memset(str, c, sz);
    return fast_aligned_wordwise_64_memset(str, c, sz);
}

extern "C" inline void* memcpy(void* dst, const void* src, uint64_t sz)
{
    return slow_memcpy(dst, src, sz);
}

/*void *operator new(std::size_t size) noexcept;
void *operator new[](size_t size) noexcept;
void operator delete(void *p) noexcept;
void operator delete[](void *p) noexcept;*/


#endif
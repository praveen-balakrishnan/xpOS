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

#ifndef X86_64_H
#define X86_64_H
#include "print.h"
namespace X86_64
{
    static constexpr uint64_t INTERRUPT_FLAG = 0x0200;

    static inline uint64_t read_rflags()
    {
        uint64_t rflags;
        asm volatile ("pushfq; popq %0" : "=r" (rflags));
        return rflags;
    }

    static inline void cli()
    {
        asm volatile ("cli");
    }

    static inline void sti()
    {
        asm volatile ("sti");
    }

    static inline void hlt()
    {
        asm volatile ("hlt");
    }

    static inline uint16_t get_cs()
    {
        uint16_t cs;
        asm volatile ("mov %%CS, %0" : "=r"(cs));
        return cs;
    }
}


#endif
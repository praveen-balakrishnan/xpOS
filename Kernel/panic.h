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

#ifndef PANIC_H
#define PANIC_H

#include "print.h"
#include "x86_64.h"

#define KERNEL_ASSERT(EX) (void)((EX) || (Kernel::__kassert (#EX, __FILE__, __LINE__),0))

namespace Kernel
{
    inline void stop_execution()
    {
        X86_64::cli();
        X86_64::hlt();
    }

    inline void panic(const char* str)
    {
        printf("!PANIC!: ");
        printf(str);
        stop_execution();
    }

    inline void __kassert(const char* msg, const char* file, int line)
    {
        printf("\n");
        printf("Assertion failed: ");
        printf(msg);
        printf(" at ");
        printf(file);
        printf(", line ");
        printf(line);
        stop_execution();
    }
}

#endif
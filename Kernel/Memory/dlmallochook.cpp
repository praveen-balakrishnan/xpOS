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

#include "Memory/dlmallochook.h"
#include "Memory/MemoryManager.h"

static uint8_t* current_base =  reinterpret_cast<uint8_t*>(0xFFFFDF8000000000);

void *replacement_sbrk(long size)
{
    auto ret = current_base;
    if (size == 0)
        return current_base;
    if (size >= 0) {
        for (; current_base < ret + size; current_base += 0x1000) {
            Memory::Manager::instance().alloc_page(Memory::VirtualMemoryAllocationRequest(Memory::VirtualAddress(current_base), true, true));
        }
    } else {
        size = -BYTE_ALIGN_DOWN(-size, 4096);
        for (; current_base > ret - size ; current_base -= 0x1000) {
            Memory::Manager::instance().free_page(Memory::VirtualMemoryFreeRequest(current_base - 0x1000));
        }
    }
    return ret;
}

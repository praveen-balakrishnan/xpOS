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

#include "Boot/Modules/ModuleManager.h"
#include "Memory/Memory.h"
#include "Memory/MemoryManager.h"
#include "panic.h"
#include "print.h"

namespace Modules
{
    void protect_pages()
    {
        auto moduleCollection = Multiboot::MultibootTagCollection(MULTIBOOT_TAG_TYPE_MODULE);
        for (auto module = moduleCollection.begin(); module != moduleCollection.end(); ++module) {
            auto tag = reinterpret_cast<multiboot_tag_module*>(&(*module));
            Memory::Manager::instance().deinit_physical_region(tag->mod_start, tag->mod_end - tag->mod_start);
        }
    }

    void map_virtual()
    {
        auto moduleCollection = Multiboot::MultibootTagCollection(MULTIBOOT_TAG_TYPE_MODULE);
        for (auto module = moduleCollection.begin(); module != moduleCollection.end(); ++module)
        {
            auto tag = reinterpret_cast<multiboot_tag_module*>(&(*module));
            // Map modules into virtual memory.
            for (std::size_t i = 0; i < (tag->mod_end-tag->mod_start) + Memory::PAGE_4KiB; i += Memory::PAGE_4KiB) {
                auto request = Memory::VirtualMemoryMapRequest(BYTE_ALIGN_DOWN(tag->mod_start + i, Memory::PAGE_4KiB), MODULE_BASE + i);
                request.allowWrite = true;
                Memory::Manager::instance().request_virtual_map(request);
            }
        }
    }
}

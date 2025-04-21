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

#include "Filesystem/VFS.h"
#include "Loader/ELF.h"
#include "Memory/MemoryManager.h"
#include "Memory/KernelHeap.h"
#include "Pipes/Pipe.h"

namespace Loader
{
    /*
        This is a loader for ELF-64 executables - refer to the ELF-64 Specification: https://uclibc.org/docs/elf-64-gen.pdf
    */
   extern "C" void start_userspace_task_iretq(void* entryPoint, void* kstack);

    bool check_elf_header_ident(Elf64_Ehdr* header)
    {
        if (!header)
            return false;

        if (header->e_ident[EI_MAG0] != ELFMAG0 || header->e_ident[EI_MAG1] != ELFMAG1
            || header->e_ident[EI_MAG2] != ELFMAG2 || header->e_ident[EI_MAG3] != ELFMAG3) {
            return false;
        }
        return true;
    }

    bool check_elf_header_support(Elf64_Ehdr* header)
    {
        if (header->e_ident[EI_CLASS] != ELFCLASS64)
            return false;
        if (header->e_ident[EI_DATA] != ELFDATA2LSB)
            return false;
        if (header->e_machine != EM_AMD64)
            return false;
        if (header->e_ident[EI_VERSION] != EV_CURRENT)
            return false;
        if (header->e_type != ET_REL && header->e_type != ET_EXEC)
            return false;
        return true;
    }

    Elf64_Shdr* get_section_header(Elf64_Ehdr* header)
    {
        return reinterpret_cast<Elf64_Shdr*>(reinterpret_cast<uint64_t>(header) + header->e_shoff);
    }

    Elf64_Shdr* get_section(Elf64_Ehdr* header, int index)
    {
        return &get_section_header(header)[index];
    }

    void* get_string_table(Elf64_Ehdr* header)
    {
        return reinterpret_cast<uint8_t*>(header) + get_section(header, header->e_shstrndx)->sh_offset;
    }

    char* lookup_string_in_table(Elf64_Ehdr* header, int offset)
    {
        char* table = static_cast<char*>(get_string_table(header));
        return table + offset;
    }

    void prepare_elf(const char* path, Task::Task* task)
    {
        // Load the ELF file into memory.
        Pipes::Pipe pipe("vfs", const_cast<void*>(reinterpret_cast<const void*>(path)), 0);
        auto fileInfo = pipe.info();
        auto elfFile = reinterpret_cast<Elf64_Ehdr*>(kmalloc(fileInfo.size, 0));
        pipe.read(fileInfo.size, elfFile);
        
        if (!check_elf_header_ident(elfFile) || !check_elf_header_support(elfFile))
            return;

        // Iterate through ELF headers.
        auto* elfFileStart = reinterpret_cast<uint8_t*>(elfFile);
        for (auto* idx = elfFileStart + elfFile->e_shoff;
            idx < elfFileStart + elfFile->e_shoff + elfFile->e_shentsize * elfFile->e_shnum;
            idx += elfFile->e_shentsize) 
        {
            auto sectionHeader = (Elf64_Shdr*)idx;
            // If the section has a virtual address, we need to load it in.
            if (sectionHeader->sh_addr) {
                // Allocate pages for the section.
                (*task->tlTable).insert_region(Memory::MappedRegion((void*)BYTE_ALIGN_DOWN(sectionHeader->sh_addr, Memory::PAGE_4KiB), reinterpret_cast<uint8_t*>(BYTE_ALIGN_DOWN(sectionHeader->sh_addr, Memory::PAGE_4KiB)) + sectionHeader->sh_size + Memory::PAGE_4KiB*2));
                for (uint8_t* page = (uint8_t*)(BYTE_ALIGN_DOWN(sectionHeader->sh_addr, Memory::PAGE_4KiB)); (uint64_t)page < BYTE_ALIGN_DOWN(sectionHeader->sh_addr, Memory::PAGE_4KiB) + sectionHeader->sh_size + Memory::PAGE_4KiB*2; page+= Memory::PAGE_4KiB) {
                    if (!Memory::Manager::instance().get_physical_address(page, *task->tlTable).get()) {
                        auto request = Memory::VirtualMemoryAllocationRequest(page);
                        request.allowUserAccess = true;
                        request.allowWrite = true;
                        Memory::Manager::instance().alloc_page(request, *task->tlTable);
                    }
                }
                if (sectionHeader->sh_type == SHT_NOBITS) {
                    // These sections should be filled with zeros.
                    memset((void*)(sectionHeader->sh_addr), 0, sectionHeader->sh_size);
                } else {
                    // Copy the section data from the ELF file into memory.
                    memcpy((void*)(sectionHeader->sh_addr), (void *)(((uint64_t)elfFile) + sectionHeader->sh_offset), sectionHeader->sh_size);
                }
            }
        }
        // We copy the ELF file to 0x400000 for the libc.
        memcpy((void*)ELF_HEADER_COPY, elfFile, sizeof(Elf64_Ehdr));

        // Allocate a 64KiB stack at STACK_LOAD.
        (*task->tlTable).insert_region(Memory::MappedRegion((void*)STACK_LOAD,(void*)(STACK_LOAD + STACK_SIZE)));
        
        for (uint8_t* userStackPointer = (uint8_t*)STACK_LOAD; userStackPointer <= (uint8_t*)(STACK_LOAD + STACK_SIZE); userStackPointer += Memory::PAGE_4KiB) {
            if (!Memory::Manager::instance().get_physical_address(userStackPointer, *task->tlTable).get()) {
                auto request = Memory::VirtualMemoryAllocationRequest(userStackPointer);
                request.allowUserAccess = true;
                request.allowWrite = true;
                Memory::Manager::instance().alloc_page(request, *task->tlTable);
            }
        }
        task->entryPoint = (void*)(elfFile->e_entry);
        kfree(elfFile);
    }
}

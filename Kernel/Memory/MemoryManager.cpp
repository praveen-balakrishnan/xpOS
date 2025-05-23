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

#include "Memory/Address.h"
#include "Memory/MemoryManager.h"
#include "panic.h"

namespace Memory {
    
    void Manager::init_physical_region(PhysicalAddress base, uint64_t size)
    {
        auto align = reinterpret_cast<uint64_t>(base.get()) / PHYSICAL_BLOCK_SIZE;
        auto blocks = size / PHYSICAL_BLOCK_SIZE;

        for (std::size_t i = 0; i < blocks; i++)
            m_physicalBitmap.unset(align + i);
        
        m_physicalBitmap.set(0);
    }

    void Manager::deinit_physical_region(PhysicalAddress base, uint64_t size)
    {
        auto align = reinterpret_cast<uint64_t>(base.get()) / PHYSICAL_BLOCK_SIZE;
        auto blocks = size / PHYSICAL_BLOCK_SIZE;

        // We want to ensure that absolutely all of the requested area is deinitialised - we do not mind deinitialising more.
        if (size % PHYSICAL_BLOCK_SIZE)
            blocks++;

        if ((size % PHYSICAL_BLOCK_SIZE) + (reinterpret_cast<uint64_t>(base.get()) % PHYSICAL_BLOCK_SIZE) > PHYSICAL_BLOCK_SIZE)
            blocks++;

        for (std::size_t i = 0; i < blocks; i++)
            m_physicalBitmap.set(align + i);
    }

    PhysicalAddress Manager::alloc_physical_block()
    {
        LockAcquirer l(m_physicalBitmapLock); 
        auto frame = m_physicalBitmap.first_unset();
        auto physAddress = frame * PHYSICAL_BLOCK_SIZE;
        
        if (frame == 0)
            Kernel::panic("Physical memory manager unable to allocate memory, out of memory?");

        m_physicalBitmap.set(frame);

        return PhysicalAddress(physAddress);
    }

    void Manager::free_physical_block(PhysicalAddress block)
    {
        LockAcquirer l(m_physicalBitmapLock); 
        auto physAddress = reinterpret_cast<uint64_t>(block.get());
        auto frame = physAddress / PHYSICAL_BLOCK_SIZE;
        m_physicalBitmap.unset(frame);
    }

    void Manager::protect_physical_regions(PhysicalAddress kernelEnd)
    {
        // We use the Multiboot2 memory tags to find available physical memory.
        enum MultibootMemoryEntryType : uint16_t {
            RESERVED            = 0,
            AVAILABLE_RAM       = 1,
            ACPI_INFO           = 3,
            HIBERNATION_RESV    = 4,
            DEFECTIVE_RAM       = 5
        };

        m_physicalMemorySize = 0;
        std::size_t useableMemorySize = 0;
        auto mmapCollection = Multiboot::MultibootTagCollection(MULTIBOOT_TAG_TYPE_MMAP);
        auto it = mmapCollection.begin();

        if (it == mmapCollection.end())
            Kernel::panic("The physical memory manager was unable to find a memory map tag.");
        
        // Find the end of physical memory. We treat this as the physical memory size for creating the bitmap.
        // This is quite a messy way to do it but is better than the alternatives.
        auto tag = reinterpret_cast<multiboot_tag_mmap*>(&(*it));
        for (auto entry = tag->entry_start();
            reinterpret_cast<uint64_t>(entry) < reinterpret_cast<uint64_t>(tag) + tag->size;
            entry = reinterpret_cast<multiboot_mmap_entry*>((reinterpret_cast<uint64_t>(entry) + tag->entry_size))) {
            auto entryEnd = entry->addr + entry->len;
            if (entry->type == MultibootMemoryEntryType::AVAILABLE_RAM) {
                if (entryEnd > useableMemorySize)
                    useableMemorySize = entryEnd;
            }
            if (entryEnd > m_physicalMemorySize)
                m_physicalMemorySize = entryEnd;
        }

        // Calculate size of physical memory bitmap.
        auto numBits = useableMemorySize / PHYSICAL_BLOCK_SIZE;
        if (useableMemorySize % PHYSICAL_BLOCK_SIZE)
            numBits++;

        auto numBytes = numBits / PhysicalBitmap::CHAR_BIT;
        if (numBytes % PhysicalBitmap::CHAR_BIT)
            numBytes++;
        
        // We place the bitmap at the end of physical memory.
        m_physicalBitmap.initialise(VirtualAddress(PhysicalAddress(useableMemorySize - numBytes)).get(), numBits);

        auto memoryBitmapPageStart = BYTE_ALIGN_UP(useableMemorySize - numBytes, PHYSICAL_BLOCK_SIZE);
        for (auto entry = tag->entry_start();
            reinterpret_cast<uint64_t>(entry) < reinterpret_cast<uint64_t>(tag) + tag->size;
            entry = reinterpret_cast<multiboot_mmap_entry*>((reinterpret_cast<uint64_t>(entry) + tag->entry_size)))
        {
            if (entry->type == MultibootMemoryEntryType::AVAILABLE_RAM) {
                auto alignedAddress = BYTE_ALIGN_UP(entry->addr, PHYSICAL_BLOCK_SIZE);
                init_physical_region(alignedAddress, entry->len);
            }
        }
        // Deinitialise the regions used to store the memory bitmap, the kernel and the multiboot structure.
        deinit_physical_region(memoryBitmapPageStart, useableMemorySize - memoryBitmapPageStart);
        deinit_physical_region(PhysicalAddress(static_cast<uint64_t>(0)), reinterpret_cast<uint64_t>(kernelEnd.get()));
        auto multibootStructure = Multiboot::get_structure();
        deinit_physical_region(reinterpret_cast<uint64_t>(multibootStructure), multibootStructure->total_size);
    }

    void GenericEntry::set_access_flags(VirtualMemoryMapRequest request)
    {
        set_flag(Flag::PRESENT);
        
        if (request.allowUserAccess)
            set_flag(Flag::USER_ACCESS);
        
        if (request.allowWrite)
            set_flag(Flag::WRITEABLE);
        
    }

    void GenericEntry::prepare_table_for_entry(VirtualMemoryMapRequest request)
    {
        if (!get_flag(Flag::PRESENT) || get_flag(Flag::PAGE_SIZE))
        {
            PhysicalAddress newTable = Manager::instance().alloc_physical_block();
            memset(VirtualAddress(newTable).get(), 0, sizeof(GenericTable));
            set_frame(newTable);
            clear_flag(Flag::PAGE_SIZE);
        }
        set_access_flags(request);
    }

    void GenericEntry::prepare_page_for_entry(VirtualMemoryMapRequest request)
    {
        if (request.pageSize != PAGE_4KiB)
            set_flag(Flag::PAGE_SIZE);
        set_frame(request.physicalAddress);
        set_access_flags(request);
    }

    void GenericEntry::free_table()
    {
        // If the PAGE_SIZE flag is set, then this is not a table but rather a page entry.
        if (get_flag(Flag::PRESENT) && !get_flag(Flag::PAGE_SIZE))
        {
            Manager::instance().free_physical_block(get_frame());
            m_entry = 0;
        }
    }
    
    void PML4Table::request_virtual_map(VirtualMemoryMapRequest request)
    {
        auto entry = get_entry(request.virtualAddress);
        entry->prepare_table_for_entry(request);
        auto pageDirectoryPointerTable = static_cast<PageDirectoryPointerTable*>(VirtualAddress(PhysicalAddress(entry->get_frame())).get());
        pageDirectoryPointerTable->request_virtual_map(request);
    }

    void PageDirectoryPointerTable::request_virtual_map(VirtualMemoryMapRequest request)
    {
        auto entry = get_entry(request.virtualAddress);
        if (request.pageSize == PAGE_1GiB)
        {
            if (!CHECK_ALIGN(reinterpret_cast<uint64_t>(request.physicalAddress.get()), PAGE_1GiB)
            || !CHECK_ALIGN(reinterpret_cast<uint64_t>(request.virtualAddress.get()), PAGE_1GiB))
            {
                Kernel::panic("Virtual memory map request failed: Address is not 1GiB aligned.");
            }
            entry->free_table();
            entry->prepare_page_for_entry(request);
        }
        else
        {
            entry->prepare_table_for_entry(request);
            auto pageDirectory = static_cast<PageDirectoryTable*>(VirtualAddress(PhysicalAddress(entry->get_frame())).get());
            pageDirectory->request_virtual_map(request);
        }
    }

    void PageDirectoryTable::request_virtual_map(VirtualMemoryMapRequest request)
    {
        auto entry = get_entry(request.virtualAddress);
        if (request.pageSize == PAGE_2MiB)
        {
            if (!CHECK_ALIGN(reinterpret_cast<uint64_t>(request.physicalAddress.get()), PAGE_2MiB)
            || !CHECK_ALIGN(reinterpret_cast<uint64_t>(request.virtualAddress.get()), PAGE_2MiB))
            {
                Kernel::panic("Virtual memory map request failed: Address is not 2MiB aligned.");
            }
            entry->free_table();
            entry->prepare_page_for_entry(request);
        }
        else
        {
            entry->prepare_table_for_entry(request);
            auto pageTable = static_cast<PageTable*>(VirtualAddress(PhysicalAddress(entry->get_frame())).get());
            pageTable->request_virtual_map(request);
        }
    }

    void PageTable::request_virtual_map(VirtualMemoryMapRequest request)
    {
        auto entry = get_entry(request.virtualAddress);
        if (!CHECK_ALIGN(reinterpret_cast<uint64_t>(request.physicalAddress.get()), PAGE_4KiB)
            || !CHECK_ALIGN(reinterpret_cast<uint64_t>(request.virtualAddress.get()), PAGE_4KiB))
            Kernel::panic("Virtual memory map request failed: Address is not 4KiB aligned.");
        
        entry->prepare_page_for_entry(request);
    }

    void Manager::request_virtual_map(VirtualMemoryMapRequest request, VirtualAddressSpace& addressSpace)
    {
        static_cast<PML4Table*>(VirtualAddress(addressSpace.get_physical_address()).get())->request_virtual_map(request);
        Manager::instance().flush_tlb_entry(request.virtualAddress);
    }

    void Manager::request_virtual_unmap(VirtualMemoryUnmapRequest request, VirtualAddressSpace& addressSpace)
    {
        static_cast<PML4Table*>(VirtualAddress(addressSpace.get_physical_address()).get())->request_virtual_unmap(request);
        Manager::instance().flush_tlb_entry(request.virtualAddress);
    }

    void PML4Table::request_virtual_unmap(VirtualMemoryUnmapRequest request)
    {
        auto entry = get_entry(request.virtualAddress);
        if (!entry->is_present())
            Kernel::panic("Virtual memory unmap request failed: Address does not exist.");
        auto pageDirectoryPointerTable = static_cast<PageDirectoryPointerTable*>(VirtualAddress(PhysicalAddress(entry->get_frame())).get());

        if (pageDirectoryPointerTable->request_virtual_unmap(request))
        {
            entry->free_table();
        }
    }

    bool PageDirectoryPointerTable::request_virtual_unmap(VirtualMemoryUnmapRequest request)
    {
        auto entry = get_entry(request.virtualAddress);
        if (!entry->is_present())
            Kernel::panic("Virtual memory unmap request failed: Address does not exist.");

        if (request.pageSize == PAGE_1GiB) 
        {
            entry->clear();
            return is_table_empty();
        }

        auto pageDirectoryTable = static_cast<PageDirectoryTable*>(VirtualAddress(PhysicalAddress(entry->get_frame())).get());
        if (pageDirectoryTable->request_virtual_unmap(request))
            entry->free_table();
        return is_table_empty();
    }

    bool PageDirectoryTable::request_virtual_unmap(VirtualMemoryUnmapRequest request)
    {
        auto entry = get_entry(request.virtualAddress);
        if (!entry->is_present())
            Kernel::panic("Virtual memory unmap request failed: Address does not exist.");

        if (request.pageSize == PAGE_2MiB) 
        {
            entry->clear();
            return is_table_empty();
        }

        auto pageTable = static_cast<PageTable*>(VirtualAddress(PhysicalAddress(entry->get_frame())).get());
        if (pageTable->request_virtual_unmap(request))
            entry->free_table();
        return is_table_empty();
    }

    bool PageTable::request_virtual_unmap(VirtualMemoryUnmapRequest request)
    {
        auto entry = get_entry(request.virtualAddress);

        if (!entry->get_frame().get()) { Kernel::panic("Virtual memory unmap request failed: Address does not exist."); }

        entry->clear();
        return is_table_empty();
    }

    void Manager::remap_pages()
    {
        m_virtualAddressSpace = create_virtual_address_space();
        switch_to_address_space(get_main_address_space());
    }

    // TODO: allow this to accept offsets into a page
    PhysicalAddress Manager::get_physical_address(VirtualAddress virtualAddress, VirtualAddressSpace& addressSpace)
    {
        return static_cast<PML4Table*>(VirtualAddress(addressSpace.get_physical_address()).get())->get_physical_address(virtualAddress);
    }

    PhysicalAddress PML4Table::get_physical_address(VirtualAddress virtualAddress)
    {
        auto entry = get_entry(virtualAddress);
        if (!entry->is_present())
            return PhysicalAddress(static_cast<uint64_t>(0));
        auto pageDirectoryPointerTable = static_cast<PageDirectoryPointerTable*>(VirtualAddress(PhysicalAddress(entry->get_frame())).get());
        return pageDirectoryPointerTable->get_physical_address(virtualAddress);
    }

    PhysicalAddress PageDirectoryPointerTable::get_physical_address(VirtualAddress virtualAddress)
    {
        auto entry = get_entry(virtualAddress);

        if (!entry->is_present())
            return PhysicalAddress(static_cast<uint64_t>(0));
        
        if (entry->get_flag(GenericEntry::Flag::PAGE_SIZE))
            return entry->get_frame();
        
        auto pageDirectoryTable = static_cast<PageDirectoryTable*>(VirtualAddress(PhysicalAddress(entry->get_frame())).get());
        return pageDirectoryTable->get_physical_address(virtualAddress);
    }

    PhysicalAddress PageDirectoryTable::get_physical_address(VirtualAddress virtualAddress)
    {
        auto entry = get_entry(virtualAddress);

        if (!entry->is_present())
            return PhysicalAddress(static_cast<uint64_t>(0));
        
        if (entry->get_flag(GenericEntry::Flag::PAGE_SIZE))
            return entry->get_frame();
        
        auto pageTable = static_cast<PageTable*>(VirtualAddress(PhysicalAddress(entry->get_frame())).get());
        return pageTable->get_physical_address(virtualAddress);
    }

    PhysicalAddress PageTable::get_physical_address(VirtualAddress virtualAddress)
    {
        auto entry = get_entry(virtualAddress);

        if (!entry->is_present())
            return PhysicalAddress(static_cast<uint64_t>(0));

        return entry->get_frame();
    }

    void Manager::alloc_page(VirtualMemoryAllocationRequest request, VirtualAddressSpace& addressSpace)
    {
        auto physicalAddress = alloc_physical_block();
        VirtualMemoryMapRequest mapRequest = {
            .physicalAddress = physicalAddress,
            .virtualAddress = request.virtualAddress,
            .allowWrite = request.allowWrite,
            .allowUserAccess = request.allowUserAccess
        };
        request_virtual_map(mapRequest, addressSpace);
    }

    void Manager::free_page(VirtualMemoryFreeRequest request, VirtualAddressSpace& addressSpace)
    {
        //auto physicalAddress = get_physical_address(request.virtualAddress, addressSpace);
        request_virtual_unmap(VirtualMemoryUnmapRequest(request), addressSpace);
    }
} 
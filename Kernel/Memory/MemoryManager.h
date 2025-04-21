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

#ifndef PMM_H
#define PMM_H
#include "Boot/MultibootManager.h"
#include "Memory/Address.h"
#include "Tasks/Spinlock.h"
#include "print.h"

namespace Memory {

// FIXME: when we map, fail if we are overwriting a preexisting map!

/**
 * This paging code is specific to the Intel 64/x86-64/AMD64 
 * architecture. Refer to the Intel® 64 and IA-32 Architectures Software
 * Developer's Manual, Volume 3.
 * 
 * The processor uses multi-level/hierarchical page tables to translate virtual addresses
 * to physical addresses.
 */
enum PageSize : uint64_t
{
    PAGE_4KiB = 0x1000,
    PAGE_2MiB = 0x200000,
    PAGE_1GiB = 0x40000000
};

struct MappedRegion
{
    void* start;
    void* end;

    constexpr std::size_t length() const
    {
        return reinterpret_cast<uint64_t>(end) - reinterpret_cast<uint64_t>(start);
    }
};

class Manager;

/**
 * Represents a unique address space, a many-to-one mapping from virtual (logical) addresses
 * to physical memory addresses.
 */
class VirtualAddressSpace
{
friend class Manager;
public:
    VirtualAddressSpace(PhysicalAddress topLevelTable)
    : m_physicalAddress(topLevelTable) {}
    PhysicalAddress get_physical_address()
    {
        return m_physicalAddress;
    }
protected:
    PhysicalAddress m_physicalAddress;
};

struct VirtualMemoryAllocationRequest
{
    VirtualAddress virtualAddress;
    bool allowWrite = false;
    bool allowUserAccess = false;
    VirtualMemoryAllocationRequest(
        VirtualAddress virtualAddress,
            bool allowWrite = false,
            bool allowUserAccess = false
    )
    : virtualAddress(virtualAddress)
    , allowWrite(allowWrite)
    , allowUserAccess(allowUserAccess) {}
};

struct VirtualMemoryFreeRequest
{
    VirtualAddress virtualAddress;
    VirtualMemoryFreeRequest(VirtualAddress virtualAddress)
    : virtualAddress(virtualAddress) {}
};

struct VirtualMemoryMapRequest
{
    PhysicalAddress physicalAddress;
    VirtualAddress virtualAddress;
    bool allowWrite = false;
    bool allowUserAccess = false;
    PageSize pageSize = PAGE_4KiB;
};

struct VirtualMemoryUnmapRequest
{
    VirtualAddress virtualAddress;
    PageSize pageSize = PAGE_4KiB;
    VirtualMemoryUnmapRequest(VirtualAddress virtualAddress)
    : virtualAddress(virtualAddress) {}
    VirtualMemoryUnmapRequest(VirtualMemoryFreeRequest request)
    : virtualAddress(request.virtualAddress) {}
};

/**
 * Each entry in a page table has the same generic format.
*/
class GenericEntry
{
    static constexpr uint64_t ENTRY_FRAME_MASK = 0xFFFFFFFFFFFFF000;
    uint64_t m_entry = 0;
public:
    enum Flag : uint8_t
    {
        PRESENT = 0x1,
        WRITEABLE = 0x2,
        USER_ACCESS = 0x4,
        PAGE_SIZE = 0x80
    };
    bool get_flag(Flag flag) { return m_entry & flag; }
    void set_flag(Flag flag) { m_entry |= flag; }
    void clear_flag(Flag flag) { m_entry &= (~flag); }
    void set_frame(PhysicalAddress frame)
    {
        m_entry = reinterpret_cast<uint64_t>(frame.get()) | (m_entry & ~ENTRY_FRAME_MASK);
    }

    void clear() { m_entry = 0; }

    bool is_present() { return get_flag(PRESENT); }

    PhysicalAddress get_frame()
    {
        return PhysicalAddress(m_entry & ENTRY_FRAME_MASK);
    }

    void set_access_flags(VirtualMemoryMapRequest request);

    void prepare_table_for_entry(VirtualMemoryMapRequest request);

    void prepare_page_for_entry(VirtualMemoryMapRequest request);

    void free_table();
};

/**
 * Each table consists of 512 entries, which either point to another table
 * or a page.
 */
class GenericTable
{
public:
    bool is_table_empty()
    {
        uint64_t* p = (uint64_t*)m_entries;
        uint64_t sz = ENTRIES_PER_TABLE;
        while (sz--)
            if (*p++ != 0)
                return false;
        return true;
    }
protected:
    static constexpr int ENTRIES_PER_TABLE = 512;
    GenericEntry m_entries[ENTRIES_PER_TABLE] = {};
};

class [[gnu::packed]] PML4Table : public GenericTable
{
public:
    void request_virtual_map(VirtualMemoryMapRequest request);
    void request_virtual_unmap(VirtualMemoryUnmapRequest request);
    PhysicalAddress get_physical_address(VirtualAddress virtualAddress);
private:
    GenericEntry* get_entry(VirtualAddress virtualAddress)
    {
        return &m_entries[(reinterpret_cast<uint64_t>(virtualAddress.get()) >> BIT_OFFSET) & (ENTRIES_PER_TABLE - 1)];
    }
    static constexpr uint64_t BIT_OFFSET = 39;

};

class [[gnu::packed]] PageDirectoryPointerTable : public GenericTable
{
public:
    void request_virtual_map(VirtualMemoryMapRequest request);
    bool request_virtual_unmap(VirtualMemoryUnmapRequest request);
    PhysicalAddress get_physical_address(VirtualAddress virtualAddress);
private:
    GenericEntry* get_entry(VirtualAddress virtualAddress)
    {
        return &m_entries[(reinterpret_cast<uint64_t>(virtualAddress.get()) >> BIT_OFFSET) & (ENTRIES_PER_TABLE - 1)];
    }
    static constexpr uint64_t BIT_OFFSET = 30;
};

class [[gnu::packed]] PageDirectoryTable : public GenericTable
{
public:
    void request_virtual_map(VirtualMemoryMapRequest request);
    bool request_virtual_unmap(VirtualMemoryUnmapRequest request);
    PhysicalAddress get_physical_address(VirtualAddress virtualAddress);
private:
    GenericEntry* get_entry(VirtualAddress virtualAddress)
    {
        return &m_entries[(reinterpret_cast<uint64_t>(virtualAddress.get()) >> BIT_OFFSET) & (ENTRIES_PER_TABLE - 1)];
    }
    static constexpr uint64_t BIT_OFFSET = 21;
};

class [[gnu::packed]] PageTable : public GenericTable
{
public:
    void request_virtual_map(VirtualMemoryMapRequest request);
    bool request_virtual_unmap(VirtualMemoryUnmapRequest request);
    PhysicalAddress get_physical_address(VirtualAddress virtualAddress);
private:
    GenericEntry* get_entry(VirtualAddress virtualAddress)
    {
        return &m_entries[(reinterpret_cast<uint64_t>(virtualAddress.get()) >> BIT_OFFSET) & (ENTRIES_PER_TABLE - 1)];
    }
    static constexpr uint64_t BIT_OFFSET = 12;
};

class Manager
{
    friend class PML4Table;
public:
    /**
     * Protect certain regions from being overwritten by allocator.
     * This should only be called by the kernel during initialisation.
     */ 
    void protect_physical_regions(PhysicalAddress kernelEnd);
    /**
     * Mark a region of memory as free so the physical allocator can use them.
     * 
     * @param base the beginning address of the region. This must be aligned to 4KiB.
     * @param size the number of bytes of the region. This must be aligned to 4KiB.
     */ 
    void init_physical_region(PhysicalAddress base, uint64_t size);
    /**
     * Mark a region of memory as used so the physical allocator does not overwrite them.
     * 
     * @param base the beginning address of the region. This must be aligned to 4KiB.
     * @param size the number of bytes of the region. This must be aligned to 4KiB.
    */
    void deinit_physical_region(PhysicalAddress base, uint64_t size);
    /**
     * Sets up a new page table.
     * This should only be called by the kernel during initialisation.
    */
    void remap_pages();
    /**
     * Allocates a 4KiB physical block.
     * 
     * @return the physical address of the block.
    */
    PhysicalAddress alloc_physical_block();
    /**
     * Frees a previously allocated physical block.
    */
    void free_physical_block(PhysicalAddress block);
    /**
     * Maps a virtual page in an address space.
    */
    void request_virtual_map(VirtualMemoryMapRequest request, VirtualAddressSpace& addressSpace = instance().get_main_address_space());
    /**
     * Unmaps a virtual page in an address space.
    */
    void request_virtual_unmap(VirtualMemoryUnmapRequest request, VirtualAddressSpace& addressSpace = instance().get_main_address_space());
    /**
     * Allocates a 4KiB page backed by a physical block in an address space.
    */
    void alloc_page(VirtualMemoryAllocationRequest request, VirtualAddressSpace& addressSpace = instance().get_main_address_space());
    /**
     * Frees a previously allocated 4KiB page in an address space.
    */
    void free_page(VirtualMemoryFreeRequest request, VirtualAddressSpace& addressSpace = instance().get_main_address_space());
    /**
     * Get physical address that a page-aligned virtual address is mapped to.
    */
    PhysicalAddress get_physical_address(VirtualAddress virtualAddress, VirtualAddressSpace& addressSpace = instance().get_main_address_space());

    /**
     * Creates a new virtual address space and allocates a top level table.
     * Physical memory is mapped in the higher half by default.
    */
    VirtualAddressSpace create_virtual_address_space()
    {
        auto physicalAddress = alloc_physical_block();
        auto addressSpace = VirtualAddressSpace(physicalAddress);
        memset(VirtualAddress(physicalAddress).get(), 0, sizeof(PML4Table));
        auto size = 4 * PAGE_1GiB;
        if (m_physicalMemorySize > size)
            size = m_physicalMemorySize;

        for (uint64_t physicalAddress = 0; physicalAddress < 4 * PAGE_1GiB; physicalAddress += PAGE_2MiB) {
            auto request = VirtualMemoryMapRequest(PhysicalAddress(physicalAddress), VirtualAddress(PhysicalAddress(physicalAddress)));
            request.allowWrite = true;
            request.pageSize = PAGE_2MiB;
            request_virtual_map(request, addressSpace);
        }
        
        return addressSpace;
    }

    /**
     * Destroys a new virtual address space. This does not unmap mapped pages in the address space. 
    */
    void destroy_virtual_address_space(VirtualAddressSpace& addressSpace)
    {
        free_physical_block(addressSpace.get_physical_address().get());
    }

    VirtualAddressSpace& get_main_address_space()
    {
        return m_virtualAddressSpace;
    }

    void switch_to_address_space(VirtualAddressSpace& addressSpace)
    {
        // The CR3 register is used to determine the current top level page table.
        __asm__ volatile ("movq %0, %%cr3" : : "r"(reinterpret_cast<uint64_t>(addressSpace.get_physical_address().get())));
    }

    static Manager& instance()
    {
        static Manager instance;
        return instance;
    }
private:
    Manager() :m_virtualAddressSpace(static_cast<uint64_t>(0)) {}
public:
    Manager(Manager const&) = delete;
    void operator=(Manager const&) = delete;
private:
    class PhysicalBitmap
    {
    public:
        static constexpr uint8_t CHAR_BIT = 8;

        void initialise(void* bitmap, std::size_t numberOfBits)
        {
            m_bitmap = static_cast<uint8_t*>(bitmap);
            m_bitmapSize = numberOfBits / CHAR_BIT;
            if (numberOfBits % CHAR_BIT)
                m_bitmapSize++;
            
            for (std::size_t i = 0; i < m_bitmapSize; i++) {
                m_bitmap[i] = 0xFF;
            }
        }

        void set(std::size_t bit)
        {
            if (bit >= m_bitmapSize * CHAR_BIT)
                return;
            m_bitmap[bit / CHAR_BIT] |= (1 << (bit % CHAR_BIT));
        }

        void unset(std::size_t bit)
        {
            if (bit >= m_bitmapSize * CHAR_BIT)
                return;
            m_bitmap[bit / CHAR_BIT] &= ~(1 << (bit % CHAR_BIT));
        }

        std::size_t first_unset()
        {
            for (std::size_t i = 0; i < m_bitmapSize; i++) {
                if (m_bitmap[i] == 0xFF)
                    continue;
                for (uint8_t j = 0; j < CHAR_BIT; j++) {
                    if (!(m_bitmap[i] & (1<<j))) {
                        return i * CHAR_BIT + j;
                    }
                }
            }
            return 0;
        }
        PhysicalBitmap() {}
    private:
        uint8_t* m_bitmap = 0;
        std::size_t m_bitmapSize = 0;
    public:
        PhysicalBitmap(PhysicalBitmap const&) = delete;
        void operator=(PhysicalBitmap const&) = delete;
    };
    static constexpr std::size_t PHYSICAL_BLOCK_SIZE = 0x1000;
    static constexpr VirtualAddress PHYSICAL_MEM_MAP_VIRTUAL_ADDRESS = 0xFFFFFF8000000000;
    std::size_t m_physicalMemorySize;
    PhysicalBitmap m_physicalBitmap;
    VirtualAddressSpace m_virtualAddressSpace;
    Spinlock m_physicalBitmapLock;

    /**
     * Flush the Translation Lookaside Buffer for a given virtual address.
     */
    void flush_tlb_entry(VirtualAddress virtualAddress)
    {
        auto address = reinterpret_cast<uint64_t>(virtualAddress.get());
        asm volatile("invlpg (%0)" ::"r" (address) : "memory");
    }
};
}
#endif
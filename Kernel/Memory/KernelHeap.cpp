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

#include "Memory/dlmalloc.h"
#include "Memory/KernelHeap.h"
#include "Memory/MemoryManager.h"
#include "panic.h"
#include "x86_64.h"

namespace Memory::Heap {
    /**
     * For the kernel heap manager, we use a binary search tree organising free sections in the current heap and ordering them by size.
     * This means that sufficient size spaces can be quickly found and allocated.
     * Each allocation or free space in memory also has surrounding tags to quickly point to the heap and allow for a fast kmalloc/kfree.
     */

    HeapManager::HeapManager() : m_bst(Common::BinarySearchTree<IndexEntry>(reinterpret_cast<void*>(HEAP_BASE)))
    {
        m_nextIndexPagePointer = reinterpret_cast<uint8_t*>(HEAP_BASE);
        m_nextHeapPagePointer = reinterpret_cast<uint8_t*>(HEAP_BASE + HEAP_INDEX_SIZE);

        // Allocate first pages for heap and index (binary search tree).
        Memory::Manager::instance().alloc_page(VirtualMemoryAllocationRequest(VirtualAddress(m_nextIndexPagePointer), true, true));
        Memory::Manager::instance().alloc_page(VirtualMemoryAllocationRequest(VirtualAddress(m_nextHeapPagePointer), true, true));

        m_nextIndexPagePointer += PAGE_4KiB;
        m_nextHeapPagePointer += PAGE_4KiB;

        // Create the first entry which is the size of the whole page.
        auto entry = IndexEntry();
        entry.ptr = m_nextHeapPagePointer - PAGE_4KiB + sizeof(ChunkHeader);
        entry.size = Memory::PAGE_4KiB - sizeof(ChunkHeader) - sizeof(ChunkFooter);
        insert_entry_into_tree(entry);
        write_entry_tags(entry, true);
    }

    /**
     * Write header and footer tags for memory chunks in memory.
     */
    void HeapManager::write_entry_tags(IndexEntry entry, bool isHole)
    {
        uint8_t* start = entry.ptr - sizeof(ChunkHeader);
        auto heapChunkHeader = new (start) ChunkHeader();
        heapChunkHeader->isHole = isHole;
        heapChunkHeader->size = entry.size;
        auto heapChunkFooter = new (entry.ptr + entry.size) ChunkFooter();
        heapChunkFooter->size = entry.size;
    }

    void HeapManager::insert_entry_into_tree(IndexEntry entry)
    {
        // If the binary search tree page range is too small, expand it.
        if (m_bst.byte_size() + 2 * m_bst.node_size() >= reinterpret_cast<uint64_t>(m_nextIndexPagePointer - HEAP_BASE)) {
            if (m_nextIndexPagePointer >= (void*)(HEAP_BASE + HEAP_INDEX_SIZE)) {
                printf("PANIC: HEAP OVERFLOW");
                while(1);
            }
            Memory::Manager::instance().alloc_page(Memory::VirtualMemoryAllocationRequest(m_nextIndexPagePointer, true, true));
            m_nextIndexPagePointer += PAGE_4KiB;
        }
        m_bst.insert_element(entry);
    }

    void HeapManager::remove_entry_from_tree(IndexEntry entry)
    {
        m_bst.remove_element(entry);
        // If the last page of the binary search tree page range is unused, free it.
        if (m_bst.byte_size() + m_bst.node_size() < reinterpret_cast<uint64_t>(m_nextIndexPagePointer - HEAP_BASE - Memory::PAGE_4KiB))
        {
            Memory::Manager::instance().free_page(VirtualAddress((m_nextIndexPagePointer) - Memory::PAGE_4KiB));
            m_nextIndexPagePointer -= PAGE_4KiB;
        }
    }

    IndexEntry HeapManager::get_entry(ChunkHeader* header)
    {
        IndexEntry entry = IndexEntry();
        entry.ptr = reinterpret_cast<uint8_t*>(header) + sizeof(ChunkHeader);
        entry.size = header->size;
        return entry;
    }

    IndexEntry HeapManager::get_entry(ChunkFooter* footer)
    {
        IndexEntry entry = IndexEntry();
        entry.ptr = reinterpret_cast<uint8_t*>(footer) - footer->size;
        entry.size = footer->size;
        return entry;
    }

    ChunkHeader* HeapManager::get_corresponding_header(ChunkFooter* footer)
    {
        return reinterpret_cast<ChunkHeader*>(reinterpret_cast<uint8_t*>(footer) - sizeof(ChunkHeader) - footer->size);
    }

    ChunkHeader* HeapManager::get_next_header(ChunkHeader* header)
    {
        auto headerPointer = reinterpret_cast<uint8_t*>(header) + sizeof(ChunkHeader) + sizeof(ChunkFooter) + header->size;
        if (headerPointer >= m_nextHeapPagePointer)
            return nullptr;
        return reinterpret_cast<ChunkHeader*>(headerPointer);
    }

    ChunkHeader* HeapManager::get_prev_header(ChunkHeader* header)
    {
        auto footerPointer = reinterpret_cast<ChunkFooter*>(header) - 1;
        if (reinterpret_cast<uint64_t>(footerPointer) < static_cast<uint64_t>(HEAP_BASE + HEAP_INDEX_SIZE))
            return nullptr;
        return get_corresponding_header(footerPointer);
    }

    /**
     * Expand the heap by adding an extra page at the end and merging the previous chunk if it was a hole.
     */ 
    void HeapManager::expand()
    {
        Memory::Manager::instance().alloc_page(VirtualMemoryAllocationRequest(m_nextHeapPagePointer, true, true));

        auto oldFooter = reinterpret_cast<ChunkFooter*>(m_nextHeapPagePointer) - 1;
        auto oldHeader = get_corresponding_header(oldFooter);
        if (oldHeader->isHole)
        {
            // We can simply make the previous entry 4KiB larger.
            IndexEntry oldEntry = get_entry(oldFooter);
            remove_entry_from_tree(oldEntry);
            oldEntry.size += PAGE_4KiB;
            insert_entry_into_tree(oldEntry);
            write_entry_tags(oldEntry, true);
        } else
        {
            // We need to insert a new entry for the new page.
            IndexEntry newEntry = IndexEntry();
            newEntry.ptr = m_nextHeapPagePointer + sizeof(ChunkHeader);
            newEntry.size = PAGE_4KiB - sizeof(ChunkHeader) - sizeof(ChunkFooter);
            insert_entry_into_tree(newEntry);
            write_entry_tags(newEntry, true);
        }

        m_nextHeapPagePointer += PAGE_4KiB;
    }

    void* HeapManager::kmalloc(uint64_t size, uint64_t flags)
    {
        LockAcquirer l(m_spinlock);
        auto entry = find_smallest_entry_greater_than(size);
        // Keep expanding the heap until we find a sufficiently sized entry.
        while (!entry) {
            expand();
            entry = find_smallest_entry_greater_than(size);
        }
        IndexEntry backEntry = *entry;

        remove_entry_from_tree(*entry);
        // The entry can be split into two - the allocation and the leftover free hole.
        if (backEntry.size >= size + sizeof(ChunkFooter) + sizeof(ChunkHeader))
        {
            uint64_t oldEntrySize = backEntry.size;
            backEntry.size = size;
            IndexEntry holeEntry = backEntry;
            holeEntry.ptr += backEntry.size + sizeof(ChunkHeader) + sizeof(ChunkFooter);
            holeEntry.size = oldEntrySize - size - sizeof(ChunkHeader) - sizeof(ChunkFooter);
            write_entry_tags(holeEntry, true);
            insert_entry_into_tree(holeEntry);
        }
        write_entry_tags(backEntry, false);
        return backEntry.ptr;
    }

    ChunkHeader* HeapManager::get_header(IndexEntry entry)
    {
        return reinterpret_cast<ChunkHeader*>(entry.ptr) - 1;
    }

    // If the next entry is also a hole, combine the two into a larger entry.
    void HeapManager::merge_forwards(IndexEntry* entry)
    {
        auto nextHeader = get_next_header(get_header(*entry));
        if (nextHeader && nextHeader->isHole)
        {
            auto nextEntry = get_entry(nextHeader);
            // Remove the two smaller entries.
            remove_entry_from_tree(*entry);
            remove_entry_from_tree(nextEntry);
            entry->size += nextHeader->size + sizeof(ChunkHeader) + sizeof(ChunkFooter);
            // Insert the larger, combined entry.
            insert_entry_into_tree(*entry);
            write_entry_tags(*entry, true);
        }
    }

    // If the previous entry is also a hole, combine the two into a larger entry.
    void HeapManager::merge_backwards(IndexEntry* entry)
    {
        auto prevHeader = get_prev_header(get_header(*entry));
        if (prevHeader && prevHeader->isHole)
        {
            auto prevEntry = get_entry(prevHeader);
            // Remove the two smaller entries.
            remove_entry_from_tree(*entry);
            remove_entry_from_tree(prevEntry);
            entry->size += prevHeader->size + sizeof(ChunkHeader) + sizeof(ChunkFooter);
            entry->ptr -= prevHeader->size + sizeof(ChunkHeader) + sizeof(ChunkFooter);
            // Insert the larger, combined entry.
            insert_entry_into_tree(*entry);
            write_entry_tags(*entry, true);
        }
    }

    void HeapManager::kfree(void* ptr)
    {
        LockAcquirer l(m_spinlock);
        auto header = reinterpret_cast<ChunkHeader*>(ptr) - 1;
        // Add the entry into the index binary search tree.
        auto entry = get_entry(header);
        insert_entry_into_tree(entry);
        write_entry_tags(entry, true);

        // Try and merge with holes either side, if they exist.
        merge_forwards(&entry);
        merge_backwards(&entry);
    }

    IndexEntry* HeapManager::find_smallest_entry_greater_than(uint64_t size)
    {
        // We insert a searchEntry that is of the requested size and find the successor to the node in the binary search tree.
        // The successor is the smallest entry that is greater than the searchEntry.
        if (m_bst.byte_size() + 5 * m_bst.node_size() >= reinterpret_cast<uint64_t>(m_nextIndexPagePointer - HEAP_BASE)) {
            Memory::Manager::instance().alloc_page(VirtualMemoryAllocationRequest(m_nextIndexPagePointer, true, true));
            m_nextIndexPagePointer += PAGE_4KiB;
        }
        IndexEntry searchEntry;
        searchEntry.size = size;
        searchEntry.ptr = 0;
        auto retEntry = m_bst.successor(searchEntry);
        if (m_bst.byte_size() + 5 * m_bst.node_size() < reinterpret_cast<uint64_t>(m_nextIndexPagePointer - HEAP_BASE - PAGE_4KiB)) {
            Memory::Manager::instance().free_page(VirtualMemoryFreeRequest(m_nextIndexPagePointer - PAGE_4KiB));
            m_nextIndexPagePointer -= PAGE_4KiB;
        }
        return retEntry;
    }
}

static Spinlock kmlock;

void* kmalloc(uint64_t size, uint64_t flags)
{
    LockAcquirer l(kmlock);
    return dlmalloc(size);
    //return Memory::Heap::HeapManager::instance().kmalloc(size, flags);
}

void kfree(void* ptr)
{
    LockAcquirer l(kmlock);
    return dlfree(ptr);
    //Memory::Heap::HeapManager::instance().kfree(ptr);
}

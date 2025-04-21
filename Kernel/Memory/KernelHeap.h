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

#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>

#include "Common/BinarySearchTree.h"
#include "Tasks/Spinlock.h"
#include "print.h"

namespace Memory::Heap {
    
/**
 * The heap allows for the kernel to make arbitrarily-sized allocations for kernel objects.
 */
enum Flag : uint8_t
{
    WORD_ALIGN = 0x1
};

/**
 * The ChunkHeader is written in the heap before each allocation or hole.
 */
struct [[gnu::packed]] ChunkHeader
{
    bool isHole;
    uint64_t size;
};

/**
 * The ChunkFooter is written in memory after each allocation or hole.
 */
struct [[gnu::packed]] ChunkFooter
{
    uint64_t size;
};

/**
 * Each hole is inserted into a binary search tree table organised by size.
 */
struct IndexEntry
{
    uint64_t size;
    uint8_t* ptr;

    bool operator==(const IndexEntry& entry) const = default;

    bool operator!=(const IndexEntry& entry) const = default;

    bool operator<(const IndexEntry& entry) const
    {
        return size < entry.size;
    }

    bool operator<=(const IndexEntry& entry) const
    {
        return size <= entry.size;
    }

    bool operator>(const IndexEntry& entry) const
    {
        return size > entry.size;
    }

    bool operator>=(const IndexEntry& entry) const
    {
        return size >= entry.size;
    }

}__attribute__((packed));

class HeapManager
{
public:
    void* kmalloc(uint64_t size, uint64_t flags);
    void kfree(void* ptr);
    void verify_integrity();
    static HeapManager& instance()
    {
        static HeapManager instance;
        return instance;
    }
    HeapManager(HeapManager const&) = delete;
    HeapManager& operator=(HeapManager const&) = delete;
private:
    HeapManager();
    static HeapManager* m_kernelHeapManager;

    Common::BinarySearchTree<IndexEntry> m_bst;

    // Pointer to the next page to allocate for the index (binary search tree).
    uint8_t* m_nextIndexPagePointer;
    // Pointer to the next page to allocate for the heap.
    uint8_t* m_nextHeapPagePointer;
    
    void insert_entry_into_tree(IndexEntry entry);
    void remove_entry_from_tree(IndexEntry entry);
    void write_entry_tags(IndexEntry entry, bool isHole);
    ChunkHeader* get_corresponding_header(ChunkFooter* footer);
    ChunkHeader* get_next_header(ChunkHeader* header);
    ChunkHeader* get_prev_header(ChunkHeader* header);
    IndexEntry get_entry(ChunkHeader* header);
    IndexEntry get_entry(ChunkFooter* footer);
    ChunkHeader* get_header(IndexEntry entry);
    void merge_forwards(IndexEntry* entry);
    void merge_backwards(IndexEntry* entry);
    void expand();
    IndexEntry* find_smallest_entry_greater_than(uint64_t size);

    Spinlock m_spinlock;

    static constexpr uint64_t HEAP_BASE = 0xFFFFDF8000000000;
    static constexpr uint64_t HEAP_INDEX_SIZE = 0x8000000000;
    static constexpr uint64_t HEAP_END = 0xFFFFFF8000000000;
};

}

void* kmalloc(uint64_t size, uint64_t flags = 0);

void kfree(void* ptr);
/* new and delete operators for kernel objects */

#endif
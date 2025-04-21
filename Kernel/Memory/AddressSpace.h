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

#ifndef ADDRESS_SPACE_H
#define ADDRESS_SPACE_H

#include "Common/List.h"
#include "Memory/Address.h"
#include "Memory/MemoryManager.h"

namespace Memory
{

class RegionableVirtualAddressSpace : public VirtualAddressSpace
{
public:
    RegionableVirtualAddressSpace(PhysicalAddress topLevelTable)
        : RegionableVirtualAddressSpace(VirtualAddressSpace(topLevelTable)) {}
    RegionableVirtualAddressSpace(VirtualAddressSpace addressSpace)
        : VirtualAddressSpace(addressSpace) {}

    /**
     * Finds a free space large enough and creates a new region there.
     */
    MappedRegion acquire_available_region(int64_t size);
    /**
     * Insert a new memory region that is disjoint from all other regions in
     * the address space.
     */
    void insert_region(MappedRegion region);
    /**
     * Removes a region starting at the specified address.
     */
    MappedRegion extract_region(void* start);

private:
    Common::List<MappedRegion> m_sortedRegions;
};

}

#endif

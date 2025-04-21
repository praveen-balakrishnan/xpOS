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

#include "memory/AddressSpace.h"
#include "memory/MemoryManager.h"

namespace Memory
{

void RegionableVirtualAddressSpace::insert_region(MappedRegion region)
{
    auto it = m_sortedRegions.begin();
    while (it != m_sortedRegions.end() && it->start < region.start)
        ++it;
    m_sortedRegions.insert(it, region); 
}

MappedRegion RegionableVirtualAddressSpace::extract_region(void* start)
{
    for (auto it = m_sortedRegions.begin(); it != m_sortedRegions.end(); ++it) {
        if (it->start == start) {
            auto region = *it;
            m_sortedRegions.erase(it);
            return region;
        }
    }
    return MappedRegion();
}

MappedRegion RegionableVirtualAddressSpace::acquire_available_region(int64_t size)
{
    size = BYTE_ALIGN_UP(size, PAGE_4KiB);
    void* regionBegin = reinterpret_cast<void*>(PAGE_4KiB);
    for (auto it = m_sortedRegions.begin(); it != m_sortedRegions.end();) {
        auto currentIt = it++;
        if (it == m_sortedRegions.end()) {
            regionBegin = currentIt->end;
            break;
        }
        if (size <= reinterpret_cast<uint8_t*>(it->start) - reinterpret_cast<uint8_t*>(currentIt->end))
            regionBegin = currentIt->end;
    }

    MappedRegion region(reinterpret_cast<void*>(regionBegin), reinterpret_cast<uint8_t*>(regionBegin) + size);
    insert_region(region);
    
    return region;
}

}

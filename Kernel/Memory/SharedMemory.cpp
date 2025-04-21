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

#include "API/SharedMemory.h"
#include "Common/Hashmap.h"
#include "Common/String.h"
#include "Memory/SharedMemory.h"
#include "Tasks/TaskManager.h"
#include "Pipes/Pipe.h"

namespace Memory::Shared
{

namespace
{
    class MemoryMap
    {
    public:
        MemoryMap(MappedRegion region, VirtualAddressSpace vas)
            : MemoryMap(region, vas, ARCBlock(PhysicalMemoryControlBlock(region.length())))
        {}

        MemoryMap(MappedRegion region, VirtualAddressSpace vas, MemoryMap& sharedWith)
            : MemoryMap(region, vas, sharedWith.m_physMem)
        {}

        MemoryMap(const MemoryMap& other) = default;
        MemoryMap(MemoryMap&& other) = default;

        static MemoryMap reuse_map(MappedRegion region, VirtualAddressSpace vas, MemoryMap& otherMap)
        {
            return MemoryMap(region, vas, otherMap.m_physMem);
        }

        std::size_t length() const 
        {
            return m_region.length();
        }

        friend void swap(MemoryMap& a, MemoryMap& b)
        {
            using std::swap;
            swap(a.m_region, b.m_region);
            swap(a.m_physMem, b.m_physMem);
            swap(a.m_vas, b.m_vas);
        }

        ~MemoryMap()
        {
            if (!m_physMem.has_value())
                return;
            auto regionStart = reinterpret_cast<uint64_t>(m_region.start);
            auto regionEnd = reinterpret_cast<uint64_t>(m_region.end);

            for (auto ptr = regionStart; ptr < regionEnd; ptr += PAGE_4KiB) {
                auto req = Memory::VirtualMemoryUnmapRequest(VirtualAddress(ptr));
                Memory::Manager::instance().request_virtual_unmap(req, m_vas);
            }
        }

    private:
        class PhysicalMemoryControlBlock
        {
        public:
            PhysicalMemoryControlBlock(std::size_t alignedLength)
            {
                for (uint64_t i = 0; i < alignedLength; i += PAGE_4KiB)
                    m_physicalPages.push_back(Manager::instance().alloc_physical_block());
            }

            ~PhysicalMemoryControlBlock()
            {
                for (auto page : m_physicalPages)
                    Manager::instance().free_physical_block(page);
            }

            PhysicalMemoryControlBlock(const PhysicalMemoryControlBlock&) = delete;

            PhysicalMemoryControlBlock(PhysicalMemoryControlBlock&& with)
                : PhysicalMemoryControlBlock(0)
            {
                swap(*this, with);
            }

            friend void swap(PhysicalMemoryControlBlock& a, PhysicalMemoryControlBlock& b)
            {
                using std::swap;
                swap(a.m_physicalPages, b.m_physicalPages);
            }

            const Common::List<PhysicalAddress>& get_physical_pages() const
            {
                return m_physicalPages;
            }

        private:
            Common::List<PhysicalAddress> m_physicalPages;
        };

        using ARCBlock = Common::AutomaticReferenceCountable<PhysicalMemoryControlBlock>;

        MemoryMap(MappedRegion region, VirtualAddressSpace vas, Common::Optional<ARCBlock> ref)
            : m_region(region)
            , m_physMem(ref)
            , m_vas(vas)
        {
            auto regionPtr = reinterpret_cast<uint64_t>(region.start);
            if (!m_physMem.has_value())
                return;
            
            for (auto page : (**m_physMem).get_physical_pages()) {
                Memory::VirtualMemoryMapRequest req = {
                    .physicalAddress = page,
                    .virtualAddress = regionPtr,
                    .allowWrite = true,
                    .allowUserAccess = true
                };

                Memory::Manager::instance().request_virtual_map(req, m_vas);
                regionPtr += PAGE_4KiB;
            }
        }

        MappedRegion m_region;
        Common::Optional<ARCBlock> m_physMem;
        VirtualAddressSpace m_vas;
    };

    struct MapIdentifier
    {
        Common::HashableString str;
        Common::List<MemoryMap>::Iterator mapIt;
    };

    Common::Hashmap<Common::HashableString, Common::List<MemoryMap>>* sharedMaps;
    Pipes::DeviceOperations deviceOperations;
}

void initialise()
{
    sharedMaps = new Common::Hashmap<Common::HashableString, Common::List<MemoryMap>>();
    deviceOperations = {
        .open = open,
        .close = close
    };
    Pipes::register_device("shared_mem", &deviceOperations);
}

bool open(void* with, int flags, void*& deviceSpecific)
{

    using namespace xpOS::API::SharedMemory;
    LinkRequest* linkRequest = static_cast<LinkRequest*>(with);
    auto mapIt = sharedMaps->find(linkRequest->str);
    
    if (flags & xpOS::API::SharedMemory::Flags::CREATE) {
        if (mapIt != sharedMaps->end())
            return false;
        
        mapIt = sharedMaps->insert({linkRequest->str, {}}).first;
    } else {
        if (mapIt == sharedMaps->end())
            return false;
        
        Common::List<MemoryMap>& mappingList = mapIt->second;
        linkRequest->length = mappingList.front().length();
    }

    auto task = Task::Manager::instance().get_current_task();
    auto& vas = (*task->tlTable);

    Common::List<MemoryMap>& mappingList = mapIt->second;

    MappedRegion region = vas.acquire_available_region(BYTE_ALIGN_UP(linkRequest->length, 4096));
    linkRequest->mappedTo = region.start;

    if (flags & Flags::CREATE)
        mappingList.emplace_back(region, vas);
    else
        mappingList.emplace_back(region, vas, mappingList.front());
    
    deviceSpecific = new MapIdentifier {
        .str = linkRequest->str,
        .mapIt = --mappingList.end()
    };
    return true;
}


void close(void*& deviceSpecific)
{
    auto* dev = reinterpret_cast<MapIdentifier*>(deviceSpecific);
    auto mapIt = sharedMaps->find(dev->str);

    if (mapIt == sharedMaps->end())
        return;

    Common::List<MemoryMap>& mappingList = mapIt->second;

    mappingList.erase(dev->mapIt);

    if (mappingList.size() == 0) {
        sharedMaps->erase(mapIt);
    }

    delete dev;
}

}

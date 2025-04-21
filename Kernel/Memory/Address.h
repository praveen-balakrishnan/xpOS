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

#ifndef ADDRESS_H
#define ADDRESS_H

#include <cstdint>

namespace Memory
{
class PhysicalAddress
{
public:
    constexpr PhysicalAddress() : m_physAddress(0) {}
    constexpr PhysicalAddress(uint64_t physicalAddress)
        : m_physAddress(physicalAddress) {}
    constexpr PhysicalAddress(void* physicalAddress)
        : m_physAddress(reinterpret_cast<uint64_t>(physicalAddress)) {}

    constexpr void* get()
    {
        return reinterpret_cast<void*>(m_physAddress);
    }

    constexpr uint64_t get_raw()
    {
        return m_physAddress;
    }

    constexpr bool is_aligned(std::size_t to)
    {
        return (m_physAddress & (to - 1)) == 0;
    }
    
private:
    uint64_t m_physAddress;
};

class VirtualAddress
{
public:
    constexpr VirtualAddress(uint64_t virtualAddress)
        : m_virtualAddress(virtualAddress) {}
    constexpr VirtualAddress(void* virtualAddress)
        : m_virtualAddress(reinterpret_cast<uint64_t>(virtualAddress)) {}
    constexpr VirtualAddress(PhysicalAddress physicalAddress)
        : m_virtualAddress(reinterpret_cast<uint64_t>(physicalAddress.get()) + KERNEL_PHYS_MAP_ADDRESS) {}
    
    constexpr void* get()
    {
        return reinterpret_cast<void*>(m_virtualAddress);
    }

    constexpr PhysicalAddress get_low_physical()
    {
        return PhysicalAddress(m_virtualAddress - KERNEL_PHYS_MAP_ADDRESS);
    }

    static constexpr uint64_t KERNEL_PHYS_MAP_ADDRESS = 0xFFFFFF8000000000;
private:
    uint64_t m_virtualAddress;
};
}

#endif
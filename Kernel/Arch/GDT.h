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

#ifndef GDT_H
#define GDT_H
#include <cstdint>

/**
 * This protection code is specific to the Intel 64/x86-64/AMD64 
 * architecture. Refer to the Intel® 64 and IA-32 Architectures Software
 * Developer's Manual, Volume 3.
 * 
 * The Global Descriptor Table (GDT) is a data structure that describes
 * the properties of various memory areas (segments).
 */

namespace X86_64
{
class GlobalDescriptorTable
{
public:
    struct Flag
    {
        static constexpr uint8_t AVL             = 0x1;
        static constexpr uint8_t LONG_MODE_CODE  = 0x2;
        static constexpr uint8_t DEFAULT         = 0x4;
        static constexpr uint8_t GRANULARITY     = 0x8;
    };

    struct AccessFlag
    {
        static constexpr uint8_t TSS64           = 0x09;
        static constexpr uint8_t USER            = 0x60;
        static constexpr uint8_t CODE            = 0x1A;
        static constexpr uint8_t DATA            = 0x12;
        static constexpr uint8_t PRESENT         = 0x80;
    };

    /**
     * Represents an entry into the Global Descriptor Table.
     * The base and limit describe the start and size of a memory region respectively.
     */
    class TableEntry32
    {
    private:
        uint16_t m_limit0 = 0;
        uint16_t m_base0 = 0;
        uint8_t m_base1 = 0;
        uint8_t m_access = 0;
        uint8_t m_flagsAndLimit1 = 0;
        uint8_t m_base2 = 0;
    
    public:
        TableEntry32(
            uint32_t base = 0,
            uint32_t limit = 0,
            uint8_t access = 0,
            uint8_t flags = 0
        )
        {
            set_base(base);
            set_limit(limit);
            set_access(access);
            set_flags(flags);
        }

        uint32_t get_base()
        {
            return m_base0 | (m_base1 << 16) | (m_base2 << 24);
        }

        void set_base(uint32_t base)
        {
            m_base0 = base         & 0xFFFF;
            m_base1 = (base >> 16) & 0xFF;
            m_base2 = (base >> 24) & 0xFF;
        }

        uint32_t get_limit()
        {
            return m_limit0 | ((m_flagsAndLimit1 & 0xFF) << 16);
        }

        void set_limit(uint32_t limit)
        {
            m_limit0 = limit;
            m_flagsAndLimit1 &= 0xF0;
            m_flagsAndLimit1 |= (limit & 0x0F);
        }

        uint8_t get_access()
        {
            return m_access;
        }

        void set_access(uint8_t access)
        {
            m_access = access;
        }
        
        uint8_t get_flags()
        {
            return m_flagsAndLimit1 >> 4;
        }

        void set_flags(uint8_t flags)
        {
            m_flagsAndLimit1 &= 0x0F;
            m_flagsAndLimit1 |= flags << 4;
        }

    } __attribute__((packed));

    /**
     * Represents a 64-bit entry into the Global Descriptor Table.
     * The base and limit describe the start and size of a memory region respectively.
     */
    class TableEntry64
    {
    private:
        uint16_t m_limit0 = 0;
        uint16_t m_base0 = 0;
        uint8_t m_base1 = 0;
        uint8_t m_access = 0;
        uint8_t m_flagsAndLimit1 = 0;
        uint8_t m_base2 = 0;
        uint32_t m_base3 = 0;
        uint32_t m_resv = 0;
    
    public:
        TableEntry64(
            void* base = nullptr,
            uint32_t limit = 0,
            uint8_t access = 0,
            uint8_t flags = 0
        )
        {
            set_base(base);
            set_limit(limit);
            set_access(access);
            set_flags(flags);
        }

        void* get_base()
        {
            return reinterpret_cast<void*>(m_base0 | (m_base1 << 16) | (m_base2 << 24) | (static_cast<uint64_t>(m_base3) << 32));
        }

        void set_base(void* base)
        {
            auto baseAddress = reinterpret_cast<uint64_t>(base);
            m_base0 = baseAddress         & 0xFFFF;
            m_base1 = (baseAddress >> 16) & 0xFF;
            m_base2 = (baseAddress >> 24) & 0xFF;
            m_base3 = (baseAddress >> 32) & 0xFFFFFFFF;
        }

        uint32_t get_limit()
        {
            return m_limit0 | ((m_flagsAndLimit1 & 0xFF) << 16);
        }

        void set_limit(uint32_t limit)
        {
            m_limit0 = limit;
            m_flagsAndLimit1 &= 0xF0;
            m_flagsAndLimit1 |= (limit & 0x0F);
        }

        uint8_t get_access()
        {
            return m_access;
        }

        void set_access(uint8_t access)
        {
            m_access = access;
        }
        
        uint8_t get_flags()
        {
            return m_flagsAndLimit1 >> 4;
        }

        void set_flags(uint8_t flags)
        {
            m_flagsAndLimit1 &= 0x0F;
            m_flagsAndLimit1 |= flags << 4;
        }

    } __attribute__((packed));
    /**
     * Fills in a 32-bit entry in the current Global Descriptor Table.
     */
    void set_table_entry(TableEntry32 entry, int entryNum);
    /**
     * Fills in a 64-bit entry in the current Global Descriptor Table.
     */
    void set_table_entry(TableEntry64 entry, int entryNum);
private:
    static constexpr std::size_t ENTRIES_COUNT = 7;
    TableEntry32 m_entries[ENTRIES_COUNT];
} __attribute__((packed));

struct GlobalDescriptorTableDescriptor
{
    uint16_t size;
    GlobalDescriptorTable* offset;
} __attribute__((packed));
}

#endif
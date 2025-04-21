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

#ifndef TSS_H
#define TSS_H

#include "Arch/GDT.h"

/**
 * This protection code is specific to the Intel 64/x86-64/AMD64 
 * architecture. Refer to the Intel® 64 and IA-32 Architectures Software
 * Developer's Manual, Volume 3.
 * 
 * The Task State Segment is a data structure used to store data
 * used during context switches.
 */

namespace X86_64 {

class [[gnu::packed]] TaskStateSegment
{
private:
    uint32_t m_resv0 = 0;
    uint64_t m_rsp0 = 0;
    uint64_t m_rsp1 = 0;
    uint64_t m_rsp2 = 0;
    uint64_t m_resv1 = 0;
    uint64_t m_ist1 = 0;
    uint64_t m_ist2 = 0;
    uint64_t m_ist3 = 0;
    uint64_t m_ist4 = 0;
    uint64_t m_ist5 = 0;
    uint64_t m_ist6 = 0;
    uint64_t m_ist7 = 0;
    uint64_t m_resv2 = 0;
    uint32_t m_resv3 = 0;

public:
    void* get_rsp0()
    {
        return reinterpret_cast<void*>(m_rsp0);
    }

    void set_rsp0(void* rsp0)
    {
        m_rsp0 = reinterpret_cast<uint64_t>(rsp0);
    }

    void* get_rsp1()
    {
        return reinterpret_cast<void*>(m_rsp1);
    }

    void set_rsp1(void* rsp1)
    {
        m_rsp1 = reinterpret_cast<uint64_t>(rsp1);
    }

    void* get_rsp2()
    {
        return reinterpret_cast<void*>(m_rsp2);
    }

    void set_rsp2(void* rsp2)
    {
        m_rsp1 = reinterpret_cast<uint64_t>(rsp2);
    }

};

}

#endif
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

#ifndef CPU_H
#define CPU_H

#include <cstdint>

#include "Arch/GDT.h"
#include "Arch/TSS.h"

extern "C" void asm_flush_tss();

/**
 * Represents a processor and its specific initialisation structures.
 * Modifying structures using the static methods modifies the executing processor.
 */
class CPU
{
public:
    enum class Ring : int
    {
        KERNEL = 0,
        ONE =    1,
        TWO =    2
    };

    static X86_64::GlobalDescriptorTable& get_global_descriptor_table()
    {
        return *m_gdtDescriptor.offset;
    }

    static void set_global_descriptor_table(X86_64::GlobalDescriptorTable* globalDescriptorTable);

    static X86_64::TaskStateSegment& get_task_state_segment()
    {
        return *m_taskStateSegment;
    }

    static void set_task_state_segment(X86_64::TaskStateSegment* taskStateSegment);

    static void refresh_task_state_segment()
    {
        asm_flush_tss();
    }

    static void set_ring_stack_pointer(void* stackPointer, Ring ring)
    {
        switch (ring) {
        case Ring::KERNEL:
            m_taskStateSegment->set_rsp0(stackPointer);
            break;
        case Ring::ONE:
            m_taskStateSegment->set_rsp1(stackPointer);
            break;
        case Ring::TWO:
            m_taskStateSegment->set_rsp2(stackPointer);
            break;
        }
    }
private:
    inline static X86_64::GlobalDescriptorTable* m_globalDescriptorTable;
    inline static X86_64::GlobalDescriptorTableDescriptor m_gdtDescriptor;
    inline static X86_64::TaskStateSegment* m_taskStateSegment;
    static constexpr int TSS_GDT_ENTRY_NUM = 5;
};

#endif
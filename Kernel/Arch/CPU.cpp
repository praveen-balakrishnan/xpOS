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

#include "Arch/CPU.h"

void CPU::set_global_descriptor_table(X86_64::GlobalDescriptorTable* globalDescriptorTable)
{
    m_globalDescriptorTable = globalDescriptorTable;
    m_gdtDescriptor = {
        .size = sizeof(X86_64::GlobalDescriptorTable) - 1,
        .offset = globalDescriptorTable
    };

    // Load the GDT into GDTR using the LGDT instruction.
    asm volatile ("lgdt %0" : : "m"(m_gdtDescriptor));
}


void CPU::set_task_state_segment(X86_64::TaskStateSegment* taskStateSegment)
{
    using namespace X86_64;
    m_taskStateSegment = taskStateSegment;
    // Install the TSS in the GDT.
    GlobalDescriptorTable::TableEntry64 tssGdtEntry(m_taskStateSegment, sizeof(X86_64::TaskStateSegment));
    tssGdtEntry.set_access(GlobalDescriptorTable::AccessFlag::PRESENT | GlobalDescriptorTable::AccessFlag::TSS64);
    m_globalDescriptorTable->set_table_entry(tssGdtEntry, TSS_GDT_ENTRY_NUM);
}

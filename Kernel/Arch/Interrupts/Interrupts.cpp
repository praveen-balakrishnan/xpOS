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

#include "Arch/Interrupts/Interrupts.h"
#include "Arch/Interrupts/PIC.h"
#include "Memory/MemoryManager.h"
#include "panic.h"
#include "x86_64.h"

extern void* isr_table[];

extern "C" void add_interrupt_handler(isr_callback handler, uint8_t vector)
{
    X86_64::Interrupts::Manager::instance().add_interrupt_handler(handler, vector);
}

extern "C" void remove_interrupt_handler(uint8_t vector)
{
    X86_64::Interrupts::Manager::instance().remove_interrupt_handler(vector);
}

namespace X86_64::Interrupts {

    void Manager::initialise()
    {
        m_idtr.set_base(m_idt64);
        m_idtr.set_limit(sizeof(DescriptorTableEntry) * MAX_INTERRUPTS_VECTOR - 1);
        m_codeSegmentSelector = get_cs();
        
        // Install an entry in the IDT for each PIC line.
        for (std::size_t vector = 0; vector <= MAX_PIC_VECTOR; vector++)
        {
            DescriptorTableEntry* entry = &m_idt64[vector];
            entry->set_offset(isr_table[vector]);
            entry->set_code_segment_selector(m_codeSegmentSelector);
            entry->set_flags(EntryFlag::PRESENT | EntryFlag::DT64 | EntryFlag::USER);
        }

        // Load the IDT.
        asm volatile ("lidt %0" : : "m"(m_idtr));

        for (std::size_t i = 0; i < MAX_INTERRUPTS_VECTOR; i++)
        {
            m_interruptHandlerTable[i] = nullptr;
        }
        enable_interrupts();
    }

    void Manager::add_interrupt_handler(isr_callback callback, uint8_t vector)
    {
        // Checks if the IRQ is from the PIC.
        if (vector >= MAX_EXCEPTIONS_VECTOR && vector <= MAX_PIC_VECTOR)
        {
            PIC::clear_irq_mask(vector - MAX_EXCEPTIONS_VECTOR);
        }
        m_interruptHandlerTable[vector] = callback;
    }

    void Manager::remove_interrupt_handler(uint8_t vector)
    {
        // Checks if the IRQ is from the PIC.
        if (vector >= MAX_EXCEPTIONS_VECTOR && vector <= MAX_PIC_VECTOR)
        {
            PIC::set_irq_mask(vector - MAX_EXCEPTIONS_VECTOR);
        }
        m_interruptHandlerTable[vector] = nullptr;
    }

    void Manager::internal_interrupt_handler(uint8_t vector)
    {
        switch (vector) {
            case ExceptionVectors::PAGE_FAULT:
            // If a page fault occurs, the page faulting address is stored in CR2.
            // We get it and check if it is mapped in the main address space, if so map it in the current address space.
            {
                uint64_t pageFaultAddress;
                asm volatile ("mov %%cr2, %0" : "=r" (pageFaultAddress));
                uint64_t addressSpaceAddress;
                asm volatile ("mov %%cr3, %0" : "=r" (addressSpaceAddress));
                auto physicalAddress = Memory::Manager::instance().get_physical_address(pageFaultAddress, Memory::Manager::instance().get_main_address_space());
                // Check that the physical address exists.
                if (physicalAddress.get()) {
                    auto vaddrspace = Memory::VirtualAddressSpace(addressSpaceAddress);
                    Memory::Manager::instance().request_virtual_map(Memory::VirtualMemoryMapRequest(physicalAddress, Memory::VirtualAddress(BYTE_ALIGN_DOWN(pageFaultAddress, Memory::PAGE_4KiB))), vaddrspace);
                } else {
                    printf("PANIC (HALTING): PAGEFAULT AT");
                    printf(pageFaultAddress);
                    Kernel::stop_execution();
                }
            }
            return;
            case ExceptionVectors::GENERAL_PROTECTION:
                Kernel::panic("gpf");
        }

        if (vector >= MAX_EXCEPTIONS_VECTOR) {
            if (m_interruptHandlerTable[vector])
                m_interruptHandlerTable[vector]();
            
            if (vector <= MAX_PIC_VECTOR)
                PIC::send_end_of_interrupt(vector - MAX_EXCEPTIONS_VECTOR);
        }
    }

    void Manager::handle_noerr_interrupt(uint8_t vector)
    {
        Manager::instance().internal_interrupt_handler(vector);
    }

    void Manager::handle_err_interrupt(uint8_t vector, [[maybe_unused]] uint8_t err)
    {
        Manager::instance().internal_interrupt_handler(vector);
    }

    void Manager::enable_interrupts()
    {
        // Remap the base interrupt vectors for the master and slave PICs.
        PIC::remap_and_init_to_offsets(MASTER_PIC_VECTOR_BASE, SLAVE_PIC_VECTOR_BASE);
        X86_64::sti();
    }

    void Manager::disable_interrupts()
    {
        X86_64::cli();
    }
}


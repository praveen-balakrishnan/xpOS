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

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <cstdint>

typedef void (*isr_callback)();

/**
 * This interrupt code is specific to the Intel 64/x86-64/AMD64 
 * architecture. Refer to the Intel® 64 and IA-32 Architectures Software
 * Developer's Manual, Volume 3.
 * 
 * When an interrupt is raised, the processor stops executing the current
 * process to handle the interrupt, and then returns to the process after the
 * interrupt has been handled.
 */
namespace X86_64::Interrupts {
    enum ExceptionVectors : uint8_t
    {
        DIVIDE_ERROR        = 0,
        DEBUG               = 1,
        NMI_INTERRUPT       = 2,
        BREAKPOINT          = 3,
        OVERFLOW            = 4,
        BOUND_RANGE_EXCEED  = 5,
        INVALID_OPCODE      = 6,
        DEVICE_UNAVAIL      = 7,
        DOUBLE_FAULT        = 8,
        COPROC_SEG_OVERRUN  = 9,
        INVALID_TSS         = 10,
        SEG_NOT_PRESENT     = 11,
        STACK_SEG_FAULT     = 12,
        GENERAL_PROTECTION  = 13,
        PAGE_FAULT          = 14,
        RESERVED            = 15,
        FPU_MATH_FAULT      = 16,
        ALIGNMENT_CHECK     = 17,
        MACHINE_CHECK       = 18,
        SIMD_FP             = 19,
        VIRTUALIZATION      = 20,
        CONTROL_PROTECT     = 21
    };

    static constexpr uint64_t MAX_INTERRUPTS_VECTOR = 256;
    static constexpr uint64_t MAX_EXCEPTIONS_VECTOR = 32;
    static constexpr uint64_t MAX_PIC_VECTOR = 48;
    static constexpr uint8_t MASTER_PIC_VECTOR_BASE = 32;
    static constexpr uint8_t SLAVE_PIC_VECTOR_BASE = 40;

    class Manager
    {
    private:
        struct EntryFlag
        {
            static constexpr uint8_t PRESENT = 0x80;
            static constexpr uint8_t USER    = 0x60;
            static constexpr uint8_t DT64    = 0x0E;
        };

        /**
         * Represents an entry into the Interrupt Descriptor Table.
         * The offset describes the address of an interrupt service routine.
         */
        class [[gnu::packed]] DescriptorTableEntry
        {
        private:
            uint16_t m_offsetLow = 0;
            uint16_t m_codeSegmentSelector = 0;
            uint16_t m_flags = 0;
            uint16_t m_offsetMid = 0;
            uint32_t m_offsetHigh = 0;
            uint32_t m_reserved = 0;
        public:

            DescriptorTableEntry(
                void* offset = 0,
                uint16_t codeSegmentSelector = 0,
                uint16_t flags = 0
            )
            {
                set_offset(offset);
                set_code_segment_selector(codeSegmentSelector);
                set_flags(flags);
            }

            void* get_offset()
            {
                return reinterpret_cast<void*>((static_cast<uint64_t>(m_offsetHigh) << 32) | (m_offsetMid << 16) |  m_offsetLow);
            }

            void set_offset(void* offset)
            {
                auto offsetAddress = reinterpret_cast<uint64_t>(offset);
                m_offsetLow  = offsetAddress          & 0xFFFF;
                m_offsetMid  = (offsetAddress >> 16)  & 0xFFFF;
                m_offsetHigh = (offsetAddress >> 32)  & 0xFFFFFFFF;
            }
            
            uint16_t get_code_segment_selector()
            {
                return m_codeSegmentSelector;
            }

            void set_code_segment_selector(uint16_t codeSegmentSelector)
            {
                m_codeSegmentSelector = codeSegmentSelector;
            }

            uint8_t get_flags()
            {
                return m_flags >> 8;
            }

            void set_flags(uint8_t flags)
            { 
                m_flags = (flags << 8);
            }

        };

        class  [[gnu::packed]] DescriptorTableRegister
        {
        private:
            uint16_t m_limit;
            uint64_t m_base;
        
        public:
            uint16_t get_limit()
            {
                return m_limit;
            }

            void set_limit(uint16_t limit)
            {
                m_limit = limit;
            }

            void* get_base()
            {
                return reinterpret_cast<void*>(m_base);
            }

            void set_base(void* base)
            {
                m_base = reinterpret_cast<uint64_t>(base);
            }

        };

        __attribute__((aligned(0x10)))
        DescriptorTableEntry m_idt64[MAX_INTERRUPTS_VECTOR];
        uint16_t m_codeSegmentSelector;
        DescriptorTableRegister m_idtr;
    public:
        static Manager& instance()
        {
            static Manager instance;
            return instance;
        }
        Manager(Manager const&) = delete;
        Manager& operator=(Manager const&) = delete;

        /**
         * Initialise and enable interrupts.
         */ 
        void initialise();
        void enable_interrupts();
        void disable_interrupts();
        void internal_interrupt_handler(uint8_t vector);
        static void handle_noerr_interrupt(uint8_t vector);
        static void handle_err_interrupt(uint8_t vector, uint8_t err);
        /**
         * Adds an interrupt handler that is called whenever a specified interrupt occurs.
         * FIXME: This overwrites any other handlers for the specific interrupt line.
         */
        void add_interrupt_handler(isr_callback handler, uint8_t vector);
        /**
         * Removes interrupt handlers for the specified interrupt line.
         */
        void remove_interrupt_handler(uint8_t vector);
    private:
        Manager() {}
        isr_callback m_interruptHandlerTable[MAX_INTERRUPTS_VECTOR];
    };
}

extern "C" void add_interrupt_handler(isr_callback handler, uint8_t vector);

extern "C" void remove_interrupt_handler(uint8_t vector);

#endif
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

#include "Arch/Interrupts/PIC.h"
#include "Arch/IO/IO.h"

namespace X86_64::Interrupts::PIC
{

    void remap_and_init_to_offsets(uint8_t masterOffset, uint8_t slaveOffset)
    {
        // Issue ICW1.
        IO::out_8(MASTER_CMD_PORT, ICW1Flags::ICW1_INIT | ICW1Flags::ICW4_NEEDED); 
        IO::wait();
        IO::out_8(SLAVE_CMD_PORT, ICW1Flags::ICW1_INIT | ICW1Flags::ICW4_NEEDED); 
        IO::wait();
        // Set PIC offsets (ICW2).
        IO::out_8(MASTER_DATA_PORT, masterOffset);  
        IO::wait();
        IO::out_8(SLAVE_DATA_PORT, slaveOffset); 
        IO::wait();
        // Issue ICW3.
        IO::out_8(MASTER_DATA_PORT, MASTER_SLAVE_LINE); 
        IO::wait();
        IO::out_8(SLAVE_DATA_PORT, SLAVE_ID); 
        IO::wait();
        // ICW4, 8086 mode.
        IO::out_8(MASTER_DATA_PORT, ICW4Flags::I8086_MODE); 
        IO::wait();
        IO::out_8(SLAVE_DATA_PORT, ICW4Flags::I8086_MODE);
        IO::wait();
        // Set masks to disable all lines.
        IO::out_8(MASTER_DATA_PORT, 0xFB); 
        IO::wait();
        IO::out_8(SLAVE_DATA_PORT, 0xFF);
    }

    void send_end_of_interrupt(uint8_t vector)
    {
        if (vector >= 8)
        {
            // Send End of Interrupt (EOI) to slave PIC (if issuer).
            IO::out_8(SLAVE_CMD_PORT, END_OF_INTERRUPT); 
        }
        // Send EOI to master PIC.
        IO::out_8(MASTER_CMD_PORT, END_OF_INTERRUPT); 
    }

    void set_irq_mask(uint8_t irqLine)
    {
        uint16_t port = irqLine < IRQ_LINES ? MASTER_DATA_PORT : SLAVE_DATA_PORT;
        uint8_t imr;
        // If line is greater than number of lines on a PIC, then it is on the slave.
        if (irqLine >= IRQ_LINES)
        {
            irqLine -= IRQ_LINES;
        }
        imr = IO::in_8(port) | (1 << irqLine);
        IO::out_8(port, imr);
    }

    void clear_irq_mask(uint8_t irqLine)
    {
        uint16_t port = irqLine < IRQ_LINES ? MASTER_DATA_PORT : SLAVE_DATA_PORT;
        uint8_t imr;
        // If line is greater than number of lines on a PIC, then it is on the slave.
        if (irqLine >= IRQ_LINES)
        {
            irqLine -= IRQ_LINES;
        }
        imr = IO::in_8(port) & ~(1 << irqLine);
        IO::out_8(port, imr);
    }
}
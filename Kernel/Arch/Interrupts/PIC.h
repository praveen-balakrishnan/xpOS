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

#ifndef PIC_H
#define PIC_H

#include <cstdint>

namespace X86_64::Interrupts::PIC
{
    /**
     * This code is specific to the Intel 8259 Programmable Interrupt Controller.
     * Refer to Intel® 8259 Programmable Interrupt Controller datasheets.
     */
    namespace 
    {
        static constexpr uint16_t MASTER_CMD_PORT = 0x20;
        static constexpr uint16_t MASTER_DATA_PORT = 0x21;    
        static constexpr uint16_t SLAVE_CMD_PORT = 0xA0;
        static constexpr uint16_t SLAVE_DATA_PORT = 0xA1;

        static constexpr uint8_t MASTER_SLAVE_LINE = 0x4;
        static constexpr uint8_t SLAVE_ID = 0x2;

        static constexpr uint8_t END_OF_INTERRUPT = 0x20;
        
        static constexpr uint8_t IRQ_LINES = 8;

        struct ICW1Flags
        {
            static constexpr uint8_t ICW4_NEEDED = 0x01;
            static constexpr uint8_t SINGLE_PIC = 0x02;
            static constexpr uint8_t CALL_ADDRESS_INTERVAL_4 = 0x04;
            static constexpr uint8_t LEVEL_TRIGGERED = 0x08;
            static constexpr uint8_t ICW1_INIT = 0x10;
        };

        struct ICW4Flags
        {
            static constexpr uint8_t I8086_MODE = 0x01;
            static constexpr uint8_t AUTO_EOI = 0x02;
            static constexpr uint8_t BUFFER_MASTER = 0x04;
            static constexpr uint8_t BUFFER_MODE = 0x08;
            static constexpr uint8_t SFNM = 0x10;
        };
        
    }

    void send_end_of_interrupt(uint8_t vector);
    void remap_and_init_to_offsets(uint8_t masterOffset, uint8_t slaveOffset);
    void set_irq_mask(uint8_t irqLine);
    void clear_irq_mask(uint8_t irqLine);
}

#endif
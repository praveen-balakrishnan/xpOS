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

#include "Arch/IO/IO.h"
#include "Arch/IO/PIT.h"
#include "Arch/Interrupts/Interrupts.h"
#include "Arch/Interrupts/PIC.h"
#include "Tasks/TaskManager.h"

namespace X86_64 {
    extern "C" void send_pit_eoi();
    namespace
    {
        static constexpr uint8_t CHANNEL_0_DATA = 0x40;
        static constexpr uint8_t CHANNEL_1_DATA = 0x41;
        static constexpr uint8_t CHANNEL_2_DATA = 0x42;
        static constexpr uint8_t COMMAND_REGISTER = 0x43;
        static constexpr uint8_t TIMER_CHANNEL_0 = 0b00000000;
        static constexpr uint8_t TIMER_CHANNEL_1 = 0b01000000;
        static constexpr uint8_t TIMER_CHANNEL_2 = 0b10000000;
        static constexpr uint8_t ACCESS_MODE_LATCH_COUNT = 0b00000000;
        static constexpr uint8_t ACCESS_MODE_LO = 0b00010000;
        static constexpr uint8_t ACCESS_MODE_HI = 0b00100000;
        static constexpr uint8_t ACCESS_MODE_LOHI = 0b00110000;
        static constexpr uint8_t OPERATING_MODE_INTR = 0b00000000;
        static constexpr uint8_t OPERATING_MODE_RG = 0b00000100;
        static constexpr uint8_t BCD4DIGIT_MODE = 0b00000001;
        static constexpr uint8_t BINARY16BIT_MODE = 0b00000000;

        static constexpr uint32_t INIT_FREQUENCY = 200;
    }

    void ProgrammableIntervalTimer::initialise()
    {
        reload_count();
        set_frequency(INIT_FREQUENCY);
        Interrupts::PIC::clear_irq_mask(0);
    }

    void ProgrammableIntervalTimer::reload_count()
    {
        // Reset count
        IO::out_8(COMMAND_REGISTER, TIMER_CHANNEL_0 | ACCESS_MODE_LOHI | OPERATING_MODE_RG | BINARY16BIT_MODE);
    }

    void ProgrammableIntervalTimer::set_frequency(uint32_t frequency)
    {
        m_freq = frequency;
        uint16_t value = ((uint64_t)1193182/(uint64_t)frequency);
        IO::out_8(CHANNEL_0_DATA, value & 0xFF);
        IO::out_8(CHANNEL_0_DATA, (value >> 8)&0xFF);
    }

    void ProgrammableIntervalTimer::tick()
    {
        // 200 Hz, T = 5ms
        ProgrammableIntervalTimer::instance().m_timeSinceBootMs += 5;
        send_pit_eoi();
        if (Task::Manager::is_executing())
            Task::Manager::instance().refresh();

    }
}

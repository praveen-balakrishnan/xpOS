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

#ifndef PIT_H
#define PIT_H

#include <cstdint>

namespace X86_64 {
/**
 * This code is specific to the Intel 8253/8254 programmable interval timers.
 * 
 * The timers raise interrupts at a regular rate that can be programmed.
 */
class ProgrammableIntervalTimer {
public:
    void initialise();
    void set_frequency(uint32_t frequency);
    /**
     * This is an interrupt service routine, called by the processor.
     */ 
    static void tick();
    static ProgrammableIntervalTimer& instance()
    {
        static ProgrammableIntervalTimer instance;
        return instance;
    }
    ProgrammableIntervalTimer(ProgrammableIntervalTimer const&) = delete;
    ProgrammableIntervalTimer& operator=(ProgrammableIntervalTimer const&) = delete;
    static uint64_t time_since_boot()
    {
        return ProgrammableIntervalTimer::instance().m_timeSinceBootMs;
    }
    
private:
    ProgrammableIntervalTimer() {}
    void reload_count();
    uint32_t m_freq;
    volatile uint64_t m_timeSinceBootMs;
    volatile uint64_t m_ticks;
};
}

#endif
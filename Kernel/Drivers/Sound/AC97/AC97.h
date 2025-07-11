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

#ifndef SOUND_AC97_H
#define SOUND_AC97_H

#include <cstdint>

#include "Arch/IO/PCI.h"

namespace Drivers::Sound::AC97
{

/**
 * This code is specific to AC'97 audio codec soundcards. Refer to the Intel
 * HDA/AC'97 Programmer's Reference Manual.
 */

struct NAMRegisters
{
    static constexpr uint8_t RESET = 0x00;
    static constexpr uint8_t MASTER_VOL = 0x02;
    static constexpr uint8_t HEADPH_VOL = 0x04;
    static constexpr uint8_t MICROPH_VOL = 0x0E;
    static constexpr uint8_t PCM_OUT_VOL = 0x18;
    static constexpr uint8_t RECORD_SELECT = 0x1A;
    static constexpr uint8_t RECORD_GAIN = 0x1C;
    static constexpr uint8_t MIC_GAIN = 0x1E;
    static constexpr uint8_t EXT_AUDIO_ID = 0x28;
    static constexpr uint8_t EXT_AUDIO_CTRL = 0x2A;
    static constexpr uint8_t SRATE_PCM_FRNT = 0x2C;
    static constexpr uint8_t SRATE_PCM_SURR = 0x2E;
    static constexpr uint8_t SRATE_PCM_LFE = 0x30;
    static constexpr uint8_t SRATE_PCM_LR = 0x32;
};

struct NABMRegisters
{
    static constexpr uint8_t PCM_OUT_BDBAR = 0x10;
    static constexpr uint8_t PCM_OUT_CIV = 0x14;
    static constexpr uint8_t PCM_OUT_LVI = 0x15;
    static constexpr uint8_t PCM_OUT_SR = 0x16;
    static constexpr uint8_t PCM_OUT_PICB = 0x18;
    static constexpr uint8_t PCM_OUT_PIV = 0x1A;
    static constexpr uint8_t PCM_OUT_CR = 0x1B;
    static constexpr uint8_t GLOB_CNT = 0x2C;
};

struct GlobalControlFlags
{
    static constexpr uint32_t GPI_INTERRUPT = 1 << 0;
    static constexpr uint32_t COLD_RESET = 1 << 1;
};

struct ControlFlags
{
    static constexpr uint8_t RPBM = 1 << 0;
    static constexpr uint8_t RR = 1 << 1;
    static constexpr uint8_t LVBIE = 1 << 2;
    static constexpr uint8_t FEIE = 1 << 3;
    static constexpr uint8_t IOCE = 1 << 4;
};

struct StatusFlags
{
    static constexpr uint8_t DCH = 1 << 0;
    static constexpr uint8_t CELV = 1 << 1;
    static constexpr uint8_t LVBCI = 1 << 2;
    static constexpr uint8_t BCIS = 1 << 3;
    static constexpr uint8_t FIFOE = 1 << 4;
};

struct [[gnu::packed]] BDLEntry
{
    uint32_t bufferPointer;
    uint32_t controlAndLength;
};

class Device
{
public:
    static void initialise();

    static Device& instance()
    {
        return *m_instance;
    }

    std::size_t push_buffers(const void* data, std::size_t length);
    void start_dma();
    void stop_dma();
    void control_reset();

private:
    std::size_t push_single_buffer(const void* data, std::size_t length);

    static Device* m_instance;
    PCI::BusDevice* m_pciDev;

    static constexpr uint8_t CLASSCODE = 0x04;
    static constexpr uint8_t SUBCLASSCODE = 0x01;

    static constexpr int BDL_MAX_ENTRIES = 32;
    static constexpr int PAGE_BUFFER_COUNT = 4;

    uint16_t m_namBar;
    uint16_t m_nabmBar;
    bool m_active = false;
    int m_bdlIdx = 0;
    int m_bufferIdx = 0;

    Memory::PhysicalAddress m_buffers[PAGE_BUFFER_COUNT];

    Memory::PhysicalAddress m_bdl;
};

}

#endif
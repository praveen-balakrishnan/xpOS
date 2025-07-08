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

#include "Drivers/Sound/AC97/AC97.h"
#include "Memory/MemoryManager.h"
#include "Tasks/TaskManager.h"

namespace Drivers::Sound::AC97
{

void Device::initialise()
{
    m_instance = new Device();

    for (auto& dev : PCI::get_all_devices()) {
        if (dev.classCode == CLASSCODE
            && dev.subclassCode == SUBCLASSCODE) {
            m_instance->m_pciDev = &dev;
            break;
        }
    }
    auto* dev = m_instance->m_pciDev;
    uint32_t conf = PCI::config_read_dword(dev->bus, dev->device, dev->function, 0x4);
    conf |= 0x5;
    PCI::config_write_dword(dev->bus, dev->device, dev->function, 0x4, conf);

    auto namBar = dev->baseAddressRegisters[0].io.get_base_address();
    auto nabmBar = dev->baseAddressRegisters[1].io.get_base_address();
    m_instance->m_namBar = namBar;
    m_instance->m_nabmBar = nabmBar;

    IO::out_32(nabmBar + NABMRegisters::GLOB_CNT, GlobalControlFlags::COLD_RESET);
    IO::out_16(namBar + NAMRegisters::RESET, 0x1);
    IO::out_16(namBar + NAMRegisters::PCM_OUT_VOL, 0x0);
    IO::out_16(namBar + NAMRegisters::MASTER_VOL, 0x0);

    m_instance->m_bdl = Memory::Manager::instance().alloc_physical_block();

    for (int i = 0; i < PAGE_BUFFER_COUNT; i++)
        m_instance->m_buffers[i] = Memory::Manager::instance().alloc_physical_block();
}

std::size_t Device::push_single_buffer(const void* data, std::size_t length)
{
    if (length > Memory::PAGE_4KiB)
        return 0;

    // Wait for current buffers to be consumed if necessary.
    do {
        auto currentIdx = IO::in_8(m_nabmBar + NABMRegisters::PCM_OUT_CIV);
        auto lastValidIdx = IO::in_8(m_nabmBar + NABMRegisters::PCM_OUT_LVI);

        auto headDistance = lastValidIdx - currentIdx;
        if (headDistance < 0)
            headDistance += BDL_MAX_ENTRIES;
        
        if (m_active)
            headDistance++;
        
        // We only have at most PAGE_BUFFER_COUNT buffers, so here
        // we must have consumed all of our buffers.
        if (headDistance > PAGE_BUFFER_COUNT) {
            m_bdlIdx = currentIdx + 1;
            break;
        }

        // We have an unqueued buffer.
        if (headDistance < PAGE_BUFFER_COUNT)
            break;

    } while (m_active);

    uint16_t sampleCount = length / sizeof(uint16_t);
    auto bdl = reinterpret_cast<BDLEntry*>(Memory::VirtualAddress(m_bdl).get());
    bdl[m_bufferIdx].bufferPointer = Memory::PhysicalAddress(m_buffers + m_bufferIdx).get_raw();
    bdl[m_bufferIdx].controlAndLength = sampleCount;
    
    IO::out_32(m_nabmBar + NABMRegisters::PCM_OUT_BDBAR, m_bdl.get_raw());
    IO::out_8(m_nabmBar + NABMRegisters::PCM_OUT_CIV, m_bdlIdx);

    m_bdlIdx = (m_bdlIdx + 1) % BDL_MAX_ENTRIES;
    m_bufferIdx = (m_bufferIdx + 1) % PAGE_BUFFER_COUNT;

    return length;
}

std::size_t Device::push_buffers(const void* data, std::size_t length) {
    auto* dataPtr = static_cast<const uint8_t*>(data);
    auto remaining = length;
    
    while (remaining > 0) { 
        auto pushed = std::min(static_cast<std::size_t>(Memory::PAGE_4KiB), remaining);
        push_single_buffer(dataPtr, pushed);
        dataPtr += Memory::PAGE_4KiB;
        remaining -= pushed;
    }

    return length;
}

void Device::control_reset()
{
    IO::out_8(m_nabmBar + NABMRegisters::PCM_OUT_CR, ControlFlags::RR);

    while (IO::in_8(m_nabmBar + NABMRegisters::PCM_OUT_CR) & ControlFlags::RR)
        Task::Manager::sleep_for(50);
    
    m_active = false;
}

void Device::start_dma()
{
    auto control = IO::in_8(m_nabmBar + NABMRegisters::PCM_OUT_CR);
    control |= ControlFlags::RPBM;
    IO::out_8(m_nabmBar + NABMRegisters::PCM_OUT_CR, control);
    m_active = true;
}

void Device::stop_dma()
{
    auto control = IO::in_8(m_nabmBar + NABMRegisters::PCM_OUT_CR);
    control &= ~ControlFlags::RPBM;
    IO::out_8(m_nabmBar + NABMRegisters::PCM_OUT_CR, control);
    m_active = false;
}


}
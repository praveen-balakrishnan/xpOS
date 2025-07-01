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

    IO::out_32(nabmBar + NABMRegisters::GLOB_CNT, 0x2);
    IO::out_16(namBar + NAMRegisters::RESET, 0x1);
    IO::out_16(namBar + NAMRegisters::PCM_OUT_VOL, 0x0);
    IO::out_16(namBar + NAMRegisters::MASTER_VOL, 0x0);
}

}
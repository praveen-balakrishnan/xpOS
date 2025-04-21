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

#include "Arch/IO/PCI.h"

namespace PCI {

namespace {
    static constexpr uint16_t CONFIG_ADDRESS_PORT = 0xCF8;
    static constexpr uint16_t CONFIG_DATA_PORT = 0xCFC;
    static constexpr int BUS_COUNT = 8;
    static constexpr int DEVICE_COUNT = 32;
    static Common::List<BusDevice> devices;
}

uint32_t config_read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg)
{
    ConfigAddress address(bus, device, function, reg);
    IO::out_32(CONFIG_ADDRESS_PORT, address.get_value());
    return IO::in_32(CONFIG_DATA_PORT);
}

void config_write_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg, uint32_t value)
{
    ConfigAddress address(bus, device, function, reg);
    IO::out_32(CONFIG_ADDRESS_PORT, address.get_value());
    IO::out_32(CONFIG_DATA_PORT, value);
}

uint8_t device_functions_count(uint8_t bus, uint8_t device)
{
    // The bit checked identifies if the device is multi-functional.
    return (config_read_dword(bus, device, 0, 0x0C) & (1<<23)) ? 8 : 1;
}

BusDevice get_pci_device(uint8_t bus, uint8_t device, uint8_t function)
{
    BusDevice dev;
    dev.bus = bus;
    dev.device = device;
    dev.function = function;
    dev.vendorId = config_read_dword(bus, device, function, 0x00) & 0xFFFF;
    dev.deviceId = (config_read_dword(bus, device, function, 0x00) >> 16) & 0xFFFF;
    dev.command = config_read_dword(bus, device, function, 0x04) & 0xFFFF;
    dev.status = (config_read_dword(bus, device, function, 0x04) >> 16) & 0xFFFF;
    dev.revision = config_read_dword(bus, device, function, 0x08) & 0xF;
    dev.interruptLine = config_read_dword(bus, device, function, 0x3C) & 0xF;

    if (dev.vendorId != 0x0000 && dev.vendorId != 0xFFFF) {
        for (int barNum = 0; barNum < BusDevice::BAR_COUNT; barNum++) {
            dev.baseAddressRegisters[barNum] = config_read_dword(bus, device, function, 0x10 + 0x4 * barNum);
        }
    }
    return dev;
}

void initialise()
{
    for (int bus = 0; bus < BUS_COUNT; bus++)
    {
        for (int device = 0; device < DEVICE_COUNT; device++)
        {
            int functionCount = device_functions_count(bus, device);
            for (int func = 0; func < functionCount; func++)
            {
                auto dev = get_pci_device(bus, device, func);
                if (dev.vendorId == 0x0000 || dev.vendorId == 0xFFFF)
                    break;
                devices.push_back(dev);
            }
        }
    }
}

BusDevice* find_device(uint16_t vendorId, uint16_t deviceId)
{
    for (auto& device : devices) {
        if (device.deviceId == deviceId && device.vendorId == vendorId)
            return &device;
    }

    return nullptr;
}

const Common::List<BusDevice>& get_all_devices()
{
    return devices;
}

}
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

#ifndef PCI_H
#define PCI_H 

#include "Arch/IO/IO.h"
#include "Memory/Address.h"
#include "Common/List.h"

namespace PCI {

class MemoryBaseAddressRegister
{
public: 
    MemoryBaseAddressRegister(uint32_t value)
        : m_value(value) {}
    
    enum class Type
    {
        BIT32 = 0x0,
        RESV = 0x1,
        BIT64 = 0x2
    };

    Type get_type()
    {
        return static_cast<Type>((m_value >> 1) & 0b11);
    }

    Memory::PhysicalAddress get_low_base_address()
    {
        return m_value & 0xFFFFFFF0;
    }

private:
    uint32_t m_value;
};

class IOBaseAddressRegister
{
public:
    IOBaseAddressRegister(uint32_t value)
        : m_value(value) {}
    
    uint32_t get_base_address()
    {
        return m_value & 0xFFFFFFFC;
    }

private:
    uint32_t m_value;
};

struct BaseAddressRegister
{
    enum Type
    {
        Memory, IO
    } type;

    union
    {
        MemoryBaseAddressRegister memoryBaseAddressRegister;
        IOBaseAddressRegister ioBaseAddressRegister;
    };

    BaseAddressRegister(uint32_t value = 0)
    {
        switch (value & 0x1) {
        case 0:
            type = Memory;
            memoryBaseAddressRegister = MemoryBaseAddressRegister(value);
            break;
        case 1:
            type = IO;
            ioBaseAddressRegister = IOBaseAddressRegister(value);
            break;
        }
    }
};

struct BusDevice {
    static constexpr int BAR_COUNT = 6;

    uint8_t bus;
    uint8_t device;
    uint8_t function;

    uint16_t vendorId;
    uint16_t deviceId;
    uint8_t classCode;
    uint8_t subclassCode;
    uint16_t status;
    uint16_t command;
    uint8_t revision;
    BaseAddressRegister baseAddressRegisters[BAR_COUNT];
    uint8_t interruptLine;
};

class ConfigAddress {
public:
    ConfigAddress(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg)
    {
        set_bus(bus);
        set_device(device);
        set_function(function);
        set_reg(reg);
    }

    uint8_t get_reg()
    {
        return get_param(REG_BIT_OFFSET, REG_BIT_MASK);
    }

    void set_reg(uint8_t reg)
    {
        set_param(REG_BIT_OFFSET, REG_BIT_MASK, reg);
    }

    uint8_t get_function()
    {
        return get_param(FUNC_BIT_OFFSET, FUNC_BIT_MASK);
    }
    
    void set_function(uint8_t function)
    {
        set_param(FUNC_BIT_OFFSET, FUNC_BIT_MASK, function);
    }

    uint8_t get_device()
    {
        return get_param(DEV_BIT_OFFSET, DEV_BIT_MASK);
    }

    void set_device(uint8_t device)
    {
        set_param(DEV_BIT_OFFSET, DEV_BIT_MASK, device);
    }

    uint8_t get_bus()
    {
        return get_param(BUS_BIT_OFFSET, BUS_BIT_MASK);
    }

    void set_bus(uint8_t bus)
    {
        set_param(BUS_BIT_OFFSET, BUS_BIT_MASK, bus);
    }

    uint32_t get_value()
    {
        return m_value;
    }

private:
    static constexpr uint8_t REG_BIT_OFFSET = 0;
    static constexpr uint32_t REG_BIT_MASK = 0x000000FC;
    static constexpr uint8_t FUNC_BIT_OFFSET = 8;
    static constexpr uint32_t FUNC_BIT_MASK = 0x00000700;
    static constexpr uint8_t DEV_BIT_OFFSET = 11;
    static constexpr uint32_t DEV_BIT_MASK = 0x0000F800;
    static constexpr uint8_t BUS_BIT_OFFSET = 16;
    static constexpr uint32_t BUS_BIT_MASK = 0x00FF0000;

    inline uint8_t get_param(uint8_t bitOffset, uint32_t bitMask)
    {
        return (m_value & bitMask) >> bitOffset;
    }

    inline void set_param(uint8_t bitOffset, uint32_t bitMask, uint8_t param)
    {
        m_value = (m_value & ~bitMask) | ((param << bitOffset) & bitMask);
    }

    uint32_t m_value = 1 << 31;
};

uint32_t config_read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg);
void config_write_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg, uint32_t value);
BusDevice* find_device(uint16_t vendorId, uint16_t deviceId);
uint8_t device_functions_count(uint8_t bus, uint8_t device);
void initialise();
const Common::List<BusDevice>& get_all_devices();

}

#endif
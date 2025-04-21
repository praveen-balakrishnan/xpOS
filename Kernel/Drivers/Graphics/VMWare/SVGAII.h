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

#ifndef VMWARE_SVGAII_H
#define VMWARE_SVGAII_H

#include <cstdint>

#include "Arch/IO/PCI.h"
#include "Pipes/Pipe.h"

/**
 * This code is specific to the VMWare SVGAII Graphics Adapter. Refer to the
 * VMware SVGA Device Developer Kit.
 * 
 * For most virtualised environments, this will be the adapter of choice.
 */

namespace Drivers::Graphics::VMWareSVGAII
{

struct Command
{
    enum Type {
        FLUSH,
        SET_RESOLUTION
    } type;

    union {
    } command;
};

struct SetResolutionCommand
{
    std::size_t width;
    std::size_t height;
    int bpp;
};

struct Registers
{
    static constexpr uint32_t SVGA_REG_ID = 0;
    static constexpr uint32_t SVGA_REG_ENABLE = 1;
    static constexpr uint32_t SVGA_REG_WIDTH = 2;
    static constexpr uint32_t SVGA_REG_HEIGHT = 3;
    static constexpr uint32_t SVGA_REG_MAX_WIDTH = 4;
    static constexpr uint32_t SVGA_REG_MAX_HEIGHT = 5;
    static constexpr uint32_t SVGA_REG_BPP = 7;
    static constexpr uint32_t SVGA_REG_FB_START = 13;
    static constexpr uint32_t SVGA_REG_FB_OFFSET = 14;
    static constexpr uint32_t SVGA_REG_VRAM_SIZE = 15;
    static constexpr uint32_t SVGA_REG_FB_SIZE = 16;
    static constexpr uint32_t SVGA_REG_CAPABILITIES = 17;
    static constexpr uint32_t SVGA_REG_FIFO_START = 18;
    static constexpr uint32_t SVGA_REG_FIFO_SIZE = 19;
    static constexpr uint32_t SVGA_REG_CONFIG_DONE = 20;
    static constexpr uint32_t SVGA_REG_SYNC = 21;
    static constexpr uint32_t SVGA_REG_BUSY = 22;
};

struct [[gnu::packed]] FIFORegisters
{
    uint32_t start;
    uint32_t size;
    uint32_t next_command;
    uint32_t stop;
    //uint32_t commands[];
    volatile uint32_t* commands() volatile
    {
        return reinterpret_cast<volatile uint32_t*>(this + 1);
    }
};

struct FramebufferAccessRequest
{
    void* hint;
    void* mapped;
    std::size_t size;
};

class Device
{
public:
    void initialise();

    std::size_t get_max_width() const;
    std::size_t get_max_height() const;
    std::size_t get_width() const;
    std::size_t get_height() const;
    std::size_t get_framebuffer_size() const;
    Memory::PhysicalAddress get_framebuffer_address() const;
    std::size_t get_fifo_size() const;
    Memory::PhysicalAddress get_fifo_address() const;

    void set_screen_resolution(std::size_t width, std::size_t height);
    void flush_screen();

    static Device& instance()
    {
        static Device instance(PCI::find_device(VENDOR_ID, DEVICE_ID));
        return instance;
    }

private:
    Device(PCI::BusDevice* device);
    PCI::BusDevice* m_pciDev;
    uint16_t m_basePort;

    uint32_t read_reg(uint32_t reg) const;
    void write_reg(uint32_t reg, uint32_t value);

    void initialise_fifo();

    volatile FIFORegisters* m_fifoRegisters;
    
    /**
     * Maps the framebuffer into the virtual memory address of the calling process.
     */
    static bool open(void* with, int flags, void*& deviceSpecific);
    static void close(void*& deviceSpecific);
    /**
     * Write a command to control the device.
     */
    static std::size_t write(std::size_t offset, std::size_t count, const void* buf, void*& deviceSpecific);


    static constexpr uint16_t VENDOR_ID = 0x15AD;
    static constexpr uint16_t DEVICE_ID = 0x0405;

    static constexpr uint16_t SVGA_INDEX = 0;
    static constexpr uint16_t SVGA_VALUE = 1;
    static constexpr uint16_t SVGA_BIOS = 2;
    static constexpr uint16_t SVGA_IRQSTATUS = 8;

    static constexpr uint32_t SVGA_ID = 0x90000002;

    static inline Pipes::DeviceOperations m_deviceOperations;
};

}

#endif
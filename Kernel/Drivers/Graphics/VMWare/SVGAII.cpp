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

#include <API/Framebuffer.h>
#include "Drivers/Graphics/VMWare/SVGAII.h"

namespace Drivers::Graphics::VMWareSVGAII
{

Device::Device(PCI::BusDevice* device)
    : m_pciDev(device)
{
    /// FIXME: change PCI device objects so that it reads from the PCI bus.
    uint32_t conf = PCI::config_read_dword(m_pciDev->bus, m_pciDev->device, m_pciDev->function, 0x4);
    conf |= 0x7;
    PCI::config_write_dword(m_pciDev->bus, m_pciDev->device, m_pciDev->function, 0x4, conf);
    m_basePort = m_pciDev->baseAddressRegisters[0].io.get_base_address();

    m_deviceOperations = {
        .open = Device::open,
        .close = Device::close,
        .write = Device::write
    };

    Pipes::register_device("screen::fb", &m_deviceOperations);
}

bool Device::open(void* with, int flags, void*& deviceSpecific)
{
    auto request = reinterpret_cast<xpOS::API::Framebuffer::AccessRequest*>(with);
    auto& tlTable = *Task::Manager::instance().get_current_task()->tlTable;
    
    Memory::PhysicalAddress fbAddress = instance().get_framebuffer_address();

    auto fbAddressAlignedDown = BYTE_ALIGN_DOWN(fbAddress.get_raw(), 4096);
    // The framebuffer should always be page aligned, but we assert it anyway.
    KERNEL_ASSERT(fbAddress.get_raw() == fbAddressAlignedDown);

    // We acquire a virtual memory region sufficiently sized for the framebuffer,
    // and map it in.
    auto fbSize = instance().get_framebuffer_size();
    request->width = instance().get_width();
    request->height = instance().get_height();
    auto region = tlTable.acquire_available_region(BYTE_ALIGN_UP(fbSize, 4096));
    auto regionStart = static_cast<uint8_t*>(region.start);

    for (uint64_t mapOffset = 0; mapOffset < BYTE_ALIGN_UP(fbSize, 4096); mapOffset += Memory::PAGE_4KiB) {
        Memory::VirtualMemoryMapRequest req {
            .physicalAddress = fbAddress.get_raw() + mapOffset,
            .virtualAddress = regionStart + mapOffset,
            .allowWrite = true,
            .allowUserAccess = true
        };
        Memory::Manager::instance().request_virtual_map(req, tlTable);
    }

    auto fbbaseAddr = regionStart;
    request->mapped = fbbaseAddr;
    request->size = fbSize;
    return true;
}

void Device::close(void*& deviceSpecific)
{
    /// FIXME: Clean up allocation when framebuffer is closed.
    /// Currently the only userspace program that accesses the framebuffer
    /// should be the window server, which will not close under normal operation.
}

std::size_t Device::write(std::size_t offset, std::size_t count, const void* buf, void*& deviceSpecific)
{
    if (count != sizeof(Command))
        return 0;

    auto command = reinterpret_cast<const Command*>(buf);

    switch (command->type) {
    case Command::FLUSH:
        instance().flush_screen();
    default:
        break;
    }

    return count;
}

uint32_t Device::read_reg(uint32_t reg) const
{
    IO::out_32(m_basePort + SVGA_INDEX, reg);
    return IO::in_32(m_basePort + SVGA_VALUE);
}

void Device::write_reg(uint32_t reg, uint32_t value)
{
    IO::out_32(m_basePort + SVGA_INDEX, reg);
    IO::out_32(m_basePort + SVGA_VALUE, value);
}

void Device::initialise_fifo()
{
    // The FIFO is used to mainly to send flush commands to the device.
    auto bar0 = m_pciDev->baseAddressRegisters[2].memory;
    auto fifoAddress = bar0.get_low_base_address();

    m_fifoRegisters = reinterpret_cast<volatile FIFORegisters*>(Memory::VirtualAddress(fifoAddress).get());
    m_fifoRegisters->start = 16;
    m_fifoRegisters->size = 16 + (10 * 1024);
    m_fifoRegisters->next_command = 16;
    m_fifoRegisters->stop = 16;
}

void Device::initialise()
{
    write_reg(Registers::SVGA_REG_ID, SVGA_ID);
    initialise_fifo();
    set_screen_resolution(1280, 720);
}

void Device::flush_screen()
{
    // We send a flush command to the device by writing to its command FIFO.
    m_fifoRegisters->start = 16;
    m_fifoRegisters->size = 16 + (10 * 1024);
    m_fifoRegisters->next_command = 16 + 4 * 5;
    m_fifoRegisters->stop = 16;
    m_fifoRegisters->commands()[0] = 1;
    m_fifoRegisters->commands()[1] = 0;
    m_fifoRegisters->commands()[2] = 0;
    m_fifoRegisters->commands()[3] = get_width();
    m_fifoRegisters->commands()[4] = get_height();
    write_reg(Registers::SVGA_REG_SYNC, 1);
}

std::size_t Device::get_max_width() const
{
    return read_reg(Registers::SVGA_REG_MAX_WIDTH);
}

std::size_t Device::get_max_height() const
{
    return read_reg(Registers::SVGA_REG_MAX_HEIGHT);
}

std::size_t Device::get_width() const
{
    return read_reg(Registers::SVGA_REG_WIDTH);
}

std::size_t Device::get_height() const
{
    return read_reg(Registers::SVGA_REG_HEIGHT);
}

std::size_t Device::get_framebuffer_size() const
{
    return read_reg(Registers::SVGA_REG_FB_SIZE);
}

Memory::PhysicalAddress Device::get_framebuffer_address() const
{
    return read_reg(Registers::SVGA_REG_FB_START);
}

std::size_t Device::get_fifo_size() const
{
    return read_reg(Registers::SVGA_REG_FIFO_SIZE);
}

Memory::PhysicalAddress Device::get_fifo_address() const
{
    return read_reg(Registers::SVGA_REG_FIFO_START);
}

void Device::set_screen_resolution(std::size_t width, std::size_t height)
{
    write_reg(Registers::SVGA_REG_ENABLE, 0);
    write_reg(Registers::SVGA_REG_WIDTH, width);
    write_reg(Registers::SVGA_REG_HEIGHT, height);
    write_reg(Registers::SVGA_REG_BPP, 32);
    write_reg(Registers::SVGA_REG_ENABLE, 1);
    write_reg(Registers::SVGA_REG_CONFIG_DONE, 1);
}

}
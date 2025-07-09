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

#include "Arch/Interrupts/Interrupts.h"
#include "Drivers/Network/PCNet.h"

namespace Drivers::Network::PCNet
{

void Device::initialise(PCI::BusDevice* pciDevice)
{
    m_pciDevice = pciDevice;
    for (int bar = 0; bar < m_pciDevice->BAR_COUNT; bar++) {
        auto& reg = m_pciDevice->baseAddressRegisters[bar];
        if (reg.type == PCI::BaseAddressRegister::IO)
            m_basePort = reg.io.get_base_address();
    }
    auto interruptLine = m_pciDevice->interruptLine + X86_64::Interrupts::MASTER_PIC_VECTOR_BASE;
    add_interrupt_handler(irq_handler, interruptLine);

    // Enable I/O and bus mastering on PCI.
    uint32_t conf = PCI::config_read_dword(m_pciDevice->bus, m_pciDevice->device, m_pciDevice->function, 0x4);
    conf &= 0xFFFF0000;
    conf |= 0x5;
    PCI::config_write_dword(m_pciDevice->bus, m_pciDevice->device, m_pciDevice->function, 0x4, conf);

    uint64_t macAddress = 0;
    macAddress = IO::in_16(m_basePort + PortOffsets::APROM4);
    macAddress <<= 16;
    macAddress += IO::in_16(m_basePort + PortOffsets::APROM2);
    macAddress <<= 16;
    macAddress += IO::in_16(m_basePort + PortOffsets::APROM0);

    m_macAddr.set_raw_value(macAddress);

    // Reset the device by reading from the RESET register
    read_offset_reg(PortOffsets::RESET);
    // Enter 32 bit mode
    write_offset_reg(PortOffsets::RAP, ControlRegister::BCR20);
    write_offset_reg(PortOffsets::BDP, BCR20_SSIZE32 | BCR20_PCNET_PCI_32);
    // STOP reset
    write_offset_reg(PortOffsets::RAP, ControlRegister::CSR0);
    write_offset_reg(PortOffsets::RDP, 0x04);

    // Create initialisation block
    // We have 8 transmit and receive buffers.
    m_initBlock.set_rlen(3);
    m_initBlock.set_tlen(3);
    m_initBlock.set_mode(0x0000);
    m_initBlock.set_paddr(m_macAddr);
    m_initBlock.set_ladrf(0);

    m_txBufDescrBase = (TransmitDescriptor*)(BYTE_ALIGN_UP(reinterpret_cast<uint64_t>(m_txBufDescriptors), 16));
    m_rxBufDescrBase = (ReceiveDescriptor*)(BYTE_ALIGN_UP(reinterpret_cast<uint64_t>(m_rxBufDescriptors), 16));

    auto txBufDescrs = Memory::VirtualAddress(m_txBufDescriptors).get_low_physical();
    m_initBlock.set_tdra(txBufDescrs);
    auto rxBufDescrs = Memory::VirtualAddress(m_rxBufDescriptors).get_low_physical();
    m_initBlock.set_rdra(rxBufDescrs);

    m_txBufferBase = (uint8_t*) BYTE_ALIGN_UP(reinterpret_cast<uint64_t>(m_txBuffers), 16);
    m_rxBufferBase = (uint8_t*) BYTE_ALIGN_UP(reinterpret_cast<uint64_t>(m_rxBuffers), 16);

    for (int i = 0; i < TX_BUF_COUNT; i++) {
        auto* bufStart = &m_txBufferBase[i * BUF_SIZE];
        m_txBufDescrBase[i].TMD0 = Memory::VirtualAddress(bufStart).get_low_physical().get_raw();
        m_txBufDescrBase[i].TMD1 = ONES | BUFFER_LEN;
        m_txBufDescrBase[i].TMD2 = 0;
        m_txBufDescrBase[i].TMD3 = 0;
    }

    for (int i = 0; i < RX_BUF_COUNT; i++)
    {
        auto* bufStart = &m_rxBufferBase[i * BUF_SIZE];
        m_rxBufDescrBase[i].RMD0 = Memory::VirtualAddress(bufStart).get_low_physical().get_raw();
        m_rxBufDescrBase[i].RMD1 = OWN_BUFFER | ONES | BUFFER_LEN;
        m_rxBufDescrBase[i].RMD2 = 0;
        m_rxBufDescrBase[i].RMD3 = 0;
    }

    auto initBlockPhysAddr = Memory::VirtualAddress(&m_initBlock).get_low_physical().get_raw();

    write_offset_reg(PortOffsets::RAP, ControlRegister::CSR1);
    write_offset_reg(PortOffsets::RDP, (initBlockPhysAddr & 0xFFFF));
    write_offset_reg(PortOffsets::RAP, ControlRegister::CSR2);
    write_offset_reg(PortOffsets::RDP, ((initBlockPhysAddr >> 16) & 0xFFFF));

    write_offset_reg(PortOffsets::RAP, ControlRegister::CSR0);
    write_offset_reg(PortOffsets::RDP, CSR0_IENA | CSR0_INIT);

    write_offset_reg(PortOffsets::RAP, ControlRegister::CSR4);
    uint32_t csr4Contents = read_offset_reg(PortOffsets::RDP);
    write_offset_reg(PortOffsets::RAP, ControlRegister::CSR4);
    write_offset_reg(PortOffsets::RDP, csr4Contents | 0xC00);

    m_threadId = Task::Manager::instance().launch_kernel_process((void*)&receive_thread, this);

    // Start receiving with interrupts enabled.
    write_offset_reg(PortOffsets::RAP, ControlRegister::CSR0);
    write_offset_reg(PortOffsets::RDP, CSR0_IENA | CSR0_STRT);
}

void Device::irq_handler()
{
    instance().write_offset_reg(PortOffsets::RAP, ControlRegister::CSR0);
    uint32_t code = instance().read_offset_reg(PortOffsets::RDP);

    if (code & 0x0400)
        instance().receive_data();
    
    instance().write_offset_reg(PortOffsets::RAP, ControlRegister::CSR0);
    instance().write_offset_reg(PortOffsets::RDP, code);
}

void Device::send_data(uint8_t* buf, uint64_t size)
{
    IO::wait();
    uint8_t descriptor = m_curTxBufDescriptor;
    // Move to next transmit buffer.
    m_curTxBufDescriptor = (m_curTxBufDescriptor + 1) % TX_BUF_COUNT;
    //if (size > 1518)
    //    printf("AMD PCNET ERROR: Trying to transmit frame of size greater than 1518");
    memcpy(Memory::VirtualAddress(Memory::PhysicalAddress(m_txBufDescrBase[descriptor].TMD0)).get(), buf, size);
    m_txBufDescrBase[descriptor].TMD1 = OWN_BUFFER | ONES | STP | ENP | ((uint16_t)((-size) & 0xFFF));
    m_txBufDescrBase[descriptor].TMD2 = 0;
    m_txBufDescrBase[descriptor].TMD3 = 0;
    instance().write_offset_reg(PortOffsets::RAP, ControlRegister::CSR0);
    instance().write_offset_reg(PortOffsets::RDP, CSR0_IENA | CSR0_TDMD);
}

void Device::connect_to_network_server()
{
    auto& server = Networking::EthernetServer::instance();
    while (!server.has_started());
    server.register_device(this);
    m_server = &server;
}

void Device::receive_thread(Device* device)
{
    device->m_threadLock.acquire();

    while (true) {
        if (device->m_queuedMessages.size() > 0) {
            for (auto& message: device->m_queuedMessages) {
                //driver->m_conn->send_message(message);
                if (device->m_server.has_value())
                    (*device->m_server)->receive_from_driver(std::move(message));
            }
            device->m_queuedMessages.clear();
        }
        Task::Manager::instance().about_to_block();
        device->m_threadLock.release();
        Task::Manager::instance().block();
        device->m_threadLock.acquire();
    }
}

void Device::receive_data()
{
    // Iterate through the circular buffer descriptors.
    for (
        ;
        (m_rxBufDescrBase[m_curRxBufDescriptor].RMD1 & OWN_BUFFER) == 0
        ; m_curRxBufDescriptor = (m_curRxBufDescriptor+1)%RX_BUF_COUNT)
    {
        // Find the buffer that has been written to by the device.
        if ((m_rxBufDescrBase[m_curRxBufDescriptor].RMD1 & (STP | ENP)) == (STP | ENP))
        {
            uint32_t size = m_rxBufDescrBase[m_curRxBufDescriptor].RMD1 & 0xFFF;
            void* physBuf = reinterpret_cast<void*>(m_rxBufDescrBase[m_curRxBufDescriptor].RMD0);
            // The driver must remove the Ethernet frame check sequence, and discard any Ethernet frames which do not have the correct FCS.
            if (size > 64) { size -= 4; }

            uint8_t* recvBuf = static_cast<uint8_t*>(Memory::VirtualAddress(Memory::PhysicalAddress(physBuf)).get());

            if (m_server.has_value()) {
                Common::Vector<uint8_t> data;
                for (std::size_t i = 0; i < size; i++) {
                    data.push_back(recvBuf[i]);
                }
                m_threadLock.acquire();
                m_queuedMessages.push_back(std::move(data));
                m_threadLock.release();
                Task::Manager::instance().unblock(m_threadId);
                //m_conn->send_message(m);
            }
            /*auto eventMessage = Events::EventMessage();
            auto networkDataReceived = new NetworkDataReceived();
            networkDataReceived->buf = buf;
            networkDataReceived->size = size;
            eventMessage.message = (void*)networkDataReceived;
            
            Events::EventDispatcher::instance().dispatch_event(eventMessage, &m_self->m_listenqueue);
            Events::EventDispatcher::instance().wake_processes(&m_self->m_waitqueue);*/
        }
        m_rxBufDescrBase[m_curRxBufDescriptor].RMD1 = OWN_BUFFER | ONES | BUFFER_LEN;
    }
}

Networking::MediaAccessControlAddress Device::get_mac_address()
{
    return m_macAddr;
}

Networking::InternetProtocolAddress Device::get_ip_address()
{
    return m_ipAddr;
}

void Device::set_ip_address(Networking::InternetProtocolAddress ipAddr)
{
    m_ipAddr = ipAddr;
}

}
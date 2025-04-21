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

#ifndef DRIVERS_PCNET_H
#define DRIVERS_PCNET_H

#include <cstdint>

#include <API/Network.h>
#include <Arch/IO/PCI.h>
#include <Common/Vector.h>
#include <Networking/NetworkServer.h>
#include <Tasks/Task.h>

namespace Drivers::Network::PCNet
{

using NetworkData = Common::Vector<uint8_t>;

static constexpr int AMDPCNET_VENDOR_ID = 0x1022;
static constexpr int AMDPCNET_DEVICE_ID = 0x2000;

static constexpr int BUFFER_SIZE = 2048;

struct PortOffsets
{
    static constexpr int APROM0 = 0x00;
    static constexpr int APROM2 = 0x02;
    static constexpr int APROM4 = 0x04;
    static constexpr int RDP = 0x10;
    static constexpr int RAP = 0x12;
    static constexpr int RESET = 0x14;
    static constexpr int BDP = 0x16;
};

struct ControlRegister
{
    static constexpr int CSR0 = 0;
    static constexpr int CSR1 = 1;
    static constexpr int CSR2 = 2;
    static constexpr int CSR3 = 3;
    static constexpr int CSR4 = 4;
    static constexpr int BCR20 = 20;
};

static constexpr int BCR20_SSIZE32 = 0x100;
static constexpr int BCR20_PCNET_PCI_32 = 0x2;

static constexpr int OWN_BUFFER = 0x80000000;
static constexpr int BUFFER_LEN = 0x07FF;
static constexpr int ONES = 0xF000;
static constexpr int STP = 0x2000000;
static constexpr int ENP = 0x1000000;

static constexpr int CSR0_INIT = 0x1;
static constexpr int CSR0_STRT = 0x2;
static constexpr int CSR0_TDMD = 0x8;
static constexpr int CSR0_IENA = 0x40;

class [[gnu::packed]] InitialisationBlock
{
private:
    uint16_t m_mode;
    uint8_t m_rlen = 0;
    uint8_t m_tlen = 0;
    uint32_t m_padrLow;
    uint16_t m_padrHigh;
    uint16_t m_resv = 0;
    uint64_t m_ladrf;
    uint32_t m_rdra;
    uint32_t m_tdra;

public:
    uint16_t get_mode() const 
    {
        return m_mode;
    }

    void set_mode(uint16_t mode)
    {
        m_mode = mode;
    } 

    uint16_t get_rlen() const
    {
        return (m_rlen & 0xF0) >> 4;
    }

    void set_rlen(uint8_t rlen)
    {
        m_rlen = (rlen & 0x0F) << 4;
    }

    uint16_t get_tlen() const
    {
        return (m_tlen & 0xF0) >> 4;
    }

    void set_tlen(uint8_t tlen)
    {
        m_tlen = (tlen & 0x0F) << 4;
    }

    Networking::MediaAccessControlAddress get_paddr() const
    {
        return (static_cast<uint64_t>(m_padrHigh) << 32) | m_padrLow;
    }

    void set_paddr(Networking::MediaAccessControlAddress addr)
    {
        const uint8_t* ptr = addr.get_raw_pointer();
        m_padrHigh = (ptr[0] << 8) | ptr[1];
        m_padrLow = (ptr[2] << 24) | (ptr[3] << 16) | (ptr[4] << 6) | ptr[5];
    }

    uint64_t get_ladrf() const
    {
        return m_ladrf;
    }

    void set_ladrf(uint64_t ladrf)
    {
        m_ladrf = ladrf;
    }

    Memory::PhysicalAddress get_rdra() const
    {
        return m_rdra;
    }

    void set_rdra(Memory::PhysicalAddress addr)
    {
        m_rdra = addr.get_raw();
    }

    Memory::PhysicalAddress get_tdra() const
    {
        return m_tdra;
    }

    void set_tdra(Memory::PhysicalAddress addr)
    {
        m_tdra = addr.get_raw();
    }
};

struct [[gnu::packed]] TransmitDescriptor
{
    uint32_t TMD0;
    uint32_t TMD1;
    uint32_t TMD2;
    uint32_t TMD3;
};

struct [[gnu::packed]] ReceiveDescriptor
{
    uint32_t RMD0;
    uint32_t RMD1;
    uint32_t RMD2;
    uint32_t RMD3;
};

class Device : public EthernetNetworkDriver
{
public:
    void initialise(PCI::BusDevice* pciDevice);
    Networking::MediaAccessControlAddress get_mac_address();
    Networking::InternetProtocolAddress get_ip_address();
    void set_ip_address(Networking::InternetProtocolAddress addr);
    static void irq_handler();
    void send_data(uint8_t* buf, std::size_t size);
    void connect_to_network_server();

    static Device& instance()
    {
        static Device instance;
        return instance;
    }

private:
    Device() {}
    static constexpr int TX_BUF_COUNT = 8;
    static constexpr int RX_BUF_COUNT = 8;

    static constexpr int DESC_SIZE = sizeof(TransmitDescriptor);
    static constexpr int BUF_SIZE = 2048;

    static constexpr int TX_RING_DESC_SIZE = DESC_SIZE * TX_BUF_COUNT + 15;
    static constexpr int RX_RING_DESC_SIZE = DESC_SIZE * RX_BUF_COUNT + 15;
    
    static constexpr int TX_RING_BUF_SIZE = BUF_SIZE * TX_BUF_COUNT + 15;
    static constexpr int RX_RING_BUF_SIZE = BUF_SIZE * RX_BUF_COUNT + 15;

    uint16_t m_basePort;

    InitialisationBlock m_initBlock;
    PCI::BusDevice* m_pciDevice;

    Common::Vector<NetworkData> m_queuedMessages;

    int m_curTxBufDescriptor = 0;
    int m_curRxBufDescriptor = 0;

    TransmitDescriptor* m_txBufDescrBase;
    ReceiveDescriptor* m_rxBufDescrBase;

    uint8_t m_txBufDescriptors[TX_RING_DESC_SIZE];
    uint8_t m_rxBufDescriptors[RX_RING_DESC_SIZE];

    uint8_t* m_txBufferBase;
    uint8_t* m_rxBufferBase;

    uint8_t m_txBuffers[TX_RING_BUF_SIZE];
    uint8_t m_rxBuffers[RX_RING_BUF_SIZE];

    Networking::MediaAccessControlAddress m_macAddr;
    Networking::InternetProtocolAddress m_ipAddr;

    Common::Optional<Networking::EthernetServer*> m_server;

    void receive_data();

    Task::TaskID m_threadId;
    Spinlock m_threadLock;
    static void receive_thread(Device* device);
    
    uint16_t read_offset_reg(uint16_t offset) const
    {
        return IO::in_16(m_basePort + offset);
    }

    void write_offset_reg(uint16_t offset, uint16_t value)
    {
        IO::out_16(m_basePort + offset, value);
    }
};


}

#endif
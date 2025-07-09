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

#ifndef TCP_COMMON_H
#define TCP_COMMON_H

#include "API/Network.h"
#include "Common/CircularBuffer.h"
#include "Networking/IP/IPv4/Common.h"
#include "Pipes/Pipe.h"
#include "Tasks/WaitQueue.h"

namespace Networking::TransmissionControlProtocol
{

class [[gnu::packed]] PseudoHeader
{
public:
    InternetProtocolAddress get_source_ip() const
    {
        return m_sourceIp.get_value();
    }

    void set_source_ip(InternetProtocolAddress ip)
    {
        m_sourceIp = ip;
    }

    InternetProtocolAddress get_dest_ip() const
    {
        return m_destIp.get_value();
    }

    void set_dest_ip(InternetProtocolAddress ip)
    {
        m_destIp = ip;
    }

    uint16_t get_protocol() const
    {
        return m_protocol.get_value();
    }

    void set_protocol(uint16_t protocol)
    {
        m_protocol = protocol;
    }
    
    uint16_t get_length() const
    {
        return m_length.get_value();
    }

    void set_length(uint16_t length)
    {
        m_length = length;
    }

private:
    Utilities::BigEndian<uint32_t> m_sourceIp;
    Utilities::BigEndian<uint32_t> m_destIp;
    Utilities::BigEndian<uint16_t> m_protocol; 
    Utilities::BigEndian<uint16_t> m_length;
};

struct Flags
{
    static constexpr uint8_t FIN = 0x01;
    static constexpr uint8_t SYN = 0x02;
    static constexpr uint8_t RST = 0x04;
    static constexpr uint8_t PSH = 0x08;
    static constexpr uint8_t ACK = 0x10;
    static constexpr uint8_t URG = 0x20;
    static constexpr uint8_t ECE = 0x40;
    static constexpr uint8_t CWR = 0x80;
};

class [[gnu::packed]] Header
{
public:
    uint16_t get_source_port() const
    {
        return m_sourcePort.get_value();
    };

    void set_source_port(uint16_t port)
    {
        m_sourcePort = port;
    }

    uint16_t get_dest_port() const
    {
        return m_destPort.get_value();
    }

    void set_dest_port(uint16_t port)
    {
        m_destPort = port;
    }

    uint32_t get_sequence_number() const
    {
        return m_sequenceNumber.get_value();
    }

    void set_sequence_number(uint32_t seq)
    {
        m_sequenceNumber = seq;
    }

    uint32_t get_ack_number() const
    {
        return m_ackNumber.get_value();
    }

    void set_ack_number(uint32_t ack)
    {
        m_ackNumber = ack;
    }

    uint8_t get_data_offset() const
    {
        return m_dataOffset.get_value() >> 4;
    }

    void set_data_offset(uint8_t offset)
    {
        m_dataOffset = (offset & 0x0F) << 4;
    }

    uint8_t get_flags() const
    {
        return m_flags.get_value();
    }

    void set_flags(uint8_t flags)
    {
        m_flags = flags;
    }

    uint16_t get_window_size() const
    {
        return m_windowSize.get_value();
    }

    void set_window_size(uint16_t windowSize)
    {
        m_windowSize = windowSize;
    }

    uint16_t get_checksum() const
    {
        return m_checksum.get_value();
    }

    void set_checksum(uint16_t checksum)
    {
        m_checksum = checksum;
    }

    uint16_t get_urgent_pointer() const
    {
        return m_urgentPointer.get_value();
    }

    void set_urgent_pointer(uint16_t pointer)
    {
        m_urgentPointer = pointer;
    }

    uint32_t get_options() const
    {
        return m_options.get_value();
    }

    void set_options(uint32_t options)
    {
        m_options = options;
    }


private:
    Utilities::BigEndian<uint16_t> m_sourcePort;
    Utilities::BigEndian<uint16_t> m_destPort;
    Utilities::BigEndian<uint32_t> m_sequenceNumber;
    Utilities::BigEndian<uint32_t> m_ackNumber;
    Utilities::BigEndian<uint8_t> m_dataOffset;
    Utilities::BigEndian<uint8_t> m_flags;
    Utilities::BigEndian<uint16_t> m_windowSize;
    Utilities::BigEndian<uint16_t> m_checksum;
    Utilities::BigEndian<uint16_t> m_urgentPointer;
    Utilities::BigEndian<uint32_t> m_options;
};

}

#endif
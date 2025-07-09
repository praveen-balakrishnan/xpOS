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

#ifndef IPV4_COMMON_H
#define IPV4_COMMON_H

#include "API/Network.h"

namespace Networking::InternetProtocolV4
{

struct Version
{
    static constexpr uint8_t IPV4 = 4;
};

struct Flags
{
    static constexpr uint8_t MORE_FRAGMENTS = 0x1;
    static constexpr uint8_t NO_FRAGMENT = 0x2;
};

struct Protocol
{
    static constexpr uint8_t ICMP = 0x01;
    static constexpr uint8_t TCP = 0x06;
    static constexpr uint8_t UDP = 0x11;
};

class [[gnu::packed]] MessageHeader
{
public:
    uint8_t get_version() const
    {
        return (m_versionAndIhl.get_value() >> 4) & 0xF;
    }

    void set_version(uint8_t version)
    {
        m_versionAndIhl = (m_versionAndIhl.get_value() & 0x0F) | (version << 4);
    }

    uint8_t get_ihl() const
    {
        return m_versionAndIhl.get_value() & 0xF;
    }

    void set_ihl(uint8_t ihl)
    {
        m_versionAndIhl = (m_versionAndIhl.get_value() & 0xF0) | ihl;
    }

    uint16_t get_length() const
    {
        return m_length.get_value();
    }

    void set_length(uint16_t length)
    {
        m_length = length;
    }

    uint16_t get_ident() const
    {
        return m_ident.get_value();
    }

    void set_ident(uint16_t ident)
    {
        m_ident = ident;
    }

    uint8_t get_flags() const
    {
        return (m_flagsAndFragment.get_value() & 0xE0) >> 5;
    }

    void set_flags(uint8_t flags)
    {
        m_flagsAndFragment = flags << 5;
    }

    uint8_t get_time_to_live() const
    {
        return m_ttl.get_value();
    }

    void set_time_to_live(uint8_t ttl)
    {
        m_ttl = ttl;
    }

    uint8_t get_protocol() const
    {
        return m_protocol.get_value();
    }

    void set_protocol(uint8_t protocol)
    {
        m_protocol = protocol;
    }

    void set_fragment_offset(uint16_t fragment)
    {
        m_fragment = fragment;
    }

    uint16_t get_header_checksum() const
    {
        return m_headerChecksum.get_value();
    }

    void set_header_checksum(uint16_t checksum)
    {
        m_headerChecksum = checksum;
    }

    InternetProtocolAddress get_source_ip() const
    {
        return m_sourceAddress.get_value();
    }

    void set_source_ip(InternetProtocolAddress address)
    {
        m_sourceAddress = address;
    }

    InternetProtocolAddress get_dest_ip() const
    {
        return m_destAddress.get_value();
    }

    void set_dest_ip(InternetProtocolAddress address)
    {
        m_destAddress = address;
    }

private:
    Utilities::BigEndian<uint8_t> m_versionAndIhl;
    Utilities::BigEndian<uint8_t> m_DscpAndEcn;
    Utilities::BigEndian<uint16_t> m_length;
    Utilities::BigEndian<uint16_t> m_ident;
    Utilities::BigEndian<uint8_t> m_flagsAndFragment;
    Utilities::BigEndian<uint8_t> m_fragment;
    Utilities::BigEndian<uint8_t> m_ttl;
    Utilities::BigEndian<uint8_t> m_protocol;
    Utilities::BigEndian<uint16_t> m_headerChecksum;
    InternetProtocolAddress m_sourceAddress;
    InternetProtocolAddress m_destAddress;
};

extern InternetProtocolAddress ipAddress;
extern InternetProtocolAddress gatewayAddress;
extern std::uint32_t subnet;

void initialise(InternetProtocolAddress address, InternetProtocolAddress gateway, std::uint32_t subnetMask);

uint16_t checksum(uint16_t* data, std::size_t size);

}

#endif
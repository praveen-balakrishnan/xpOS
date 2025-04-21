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

#ifndef XPOS_API_NETWORK_H
#define XPOS_API_NETWORK_H

#include <bit>
#include <cstdint>
#include <utility>

namespace Networking
{

namespace Utilities
{
    static constexpr int MAC_ADDRESS_SIZE = 6;

    template<typename T>
    class [[gnu::packed]] NetworkEndian
    {
    public:
        constexpr NetworkEndian() = default;

        constexpr NetworkEndian(T value)
        {
            set_value(value);
        }

        constexpr T operator=(T value) 
        {
            m_value = std::byteswap(value);
            return value;
        }

        constexpr T get_value() const
        {
            return std::byteswap(m_value);
        }

        constexpr void set_value(T value)
        {
            m_value = std::byteswap(value);
        }

        constexpr void set_raw_value(T value)
        {
            m_value = value;
        }

        friend bool operator== (const NetworkEndian& lhs, const NetworkEndian& rhs)
        {
            return lhs.get_value() == rhs.get_value();
        }

    private:
        T m_value;
    };
}

using PortNumber = uint16_t;

using InternetProtocolAddress = Utilities::NetworkEndian<std::uint32_t>;

using Endpoint = std::pair<PortNumber, InternetProtocolAddress>;

class [[gnu::packed]] MediaAccessControlAddress
{
public:
    constexpr MediaAccessControlAddress() = default;
    constexpr MediaAccessControlAddress(uint64_t value)
    {
        set_value(value);
    }

    constexpr MediaAccessControlAddress operator=(uint64_t addr)
    {
        set_value(addr);
        return *this;
    }

    constexpr uint64_t get_value() const
    {
        uint64_t addr = 0;
        addr |= static_cast<uint64_t>(m_mac[0]) << 40;
        addr |= static_cast<uint64_t>(m_mac[1]) << 32;
        addr |= static_cast<uint64_t>(m_mac[2]) << 24;
        addr |= static_cast<uint64_t>(m_mac[3]) << 16;
        addr |= static_cast<uint64_t>(m_mac[4]) << 8;
        addr |= static_cast<uint64_t>(m_mac[5]) << 0;
        return addr;
    }

    constexpr void set_value(uint64_t addr)
    {
        m_mac[0] = (addr & 0xff0000000000) >> 40;
        m_mac[1] = (addr & 0x00ff00000000) >> 32;
        m_mac[2] = (addr & 0x0000ff000000) >> 24;
        m_mac[3] = (addr & 0x000000ff0000) >> 16;
        m_mac[4] = (addr & 0x00000000ff00) >> 8;
        m_mac[5] = (addr & 0x0000000000ff) >> 0;
    }

    constexpr void set_raw_value(uint64_t addr)
    {
        m_mac[0] = (addr & 0x0000000000ff) >> 0;
        m_mac[1] = (addr & 0x00000000ff00) >> 8;
        m_mac[2] = (addr & 0x000000ff0000) >> 16;
        m_mac[3] = (addr & 0x0000ff000000) >> 24;
        m_mac[4] = (addr & 0x00ff00000000) >> 32;
        m_mac[5] = (addr & 0xff0000000000) >> 40;
    }

    friend bool operator==(const MediaAccessControlAddress& lhs, const MediaAccessControlAddress& rhs)
    {
        return lhs.get_value() == rhs.get_value();
    }

    const uint8_t* get_raw_pointer() const
    {
        return m_mac;
    }

private:
    uint8_t m_mac[6];
};

}

#endif
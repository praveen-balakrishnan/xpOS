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

#ifndef ARP_COMMON_H
#define ARP_COMMON_H

#include "API/Network.h"
#include "Common/Optional.h"
#include "Common/Hashmap.h"
#include "Networking/Ethernet/Common.h"
#include "Tasks/Mutex.h"
#include "Tasks/WaitQueue.h"

namespace Networking::AddressResolutionProtocol
{

using HardwareType = Utilities::BigEndian<std::uint16_t>;
using ProtocolType = Ethernet::EtherType;

struct [[gnu::packed]] Message
{
    HardwareType hardwareType;
    ProtocolType protocolType;
    Utilities::BigEndian<std::uint8_t> hardwareAddressLen;
    Utilities::BigEndian<std::uint8_t> protocolAddressLen;
    Utilities::BigEndian<std::uint16_t> operation;
    MediaAccessControlAddress senderHardwareAddress;
    InternetProtocolAddress senderProtocolAddress;
    MediaAccessControlAddress targetHardwareAddress;
    InternetProtocolAddress targetProtocolAddress;
};

using AddressTable = Common::Hashmap<InternetProtocolAddress, MediaAccessControlAddress>;

struct HardwareTypeValues
{
    static constexpr HardwareType ETHERNET = 0x01;
};

struct HardwareAddressLengthValues
{
    static constexpr Utilities::BigEndian<std::uint8_t> ETHERNET = 0x06;
};

struct ProtocolAddressLengthValues
{
    static constexpr Utilities::BigEndian<std::uint8_t> IPV4 = 0x04;
};

struct OperationValues
{
    static constexpr Utilities::BigEndian<std::uint16_t> REQUEST = 0x1;
    static constexpr Utilities::BigEndian<std::uint16_t> REPLY = 0x2;
};

static constexpr MediaAccessControlAddress BROADCAST_MAC = 0xFFFFFFFFFFFF;

void initialise();

}

#endif
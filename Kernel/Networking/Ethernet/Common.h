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

#ifndef NETHERNET_H
#define NETHERNET_H

#include <cstdint>
#include "API/Network.h"

namespace Networking::Ethernet
{

using EtherType = Utilities::BigEndian<uint16_t>;

struct [[gnu::packed]] FrameHeader
{
    MediaAccessControlAddress macDest;
    MediaAccessControlAddress macSource;
    EtherType ethertype;
};

struct EtherTypeValues
{
    static constexpr EtherType IPV4 = 0x0800;
    static constexpr EtherType ARP = 0x0806;
};

extern MediaAccessControlAddress macAddress;

void initialise(MediaAccessControlAddress address);

}

#endif
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

#ifndef ETHERNET_SEND_H
#define ETHERNET_SEND_H

#include "Networking/Ethernet/Common.h"

namespace Networking::Ethernet
{

void send(MediaAccessControlAddress destMac, EtherType type, uint8_t* buf, std::size_t size);

}

#endif
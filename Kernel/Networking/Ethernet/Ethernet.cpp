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

#include "Networking/Ethernet/Send.h"
#include "Networking/Ethernet/Receive.h"
#include "Networking/NetworkServer.h"
#include "Networking/IP/IPv4/Receive.h"
#include "Networking/ARP/Receive.h"
#include "Memory/KernelHeap.h"
#include "print.h"

namespace Networking::Ethernet
{

MediaAccessControlAddress macAddress;

void initialise(MediaAccessControlAddress address)
{
    macAddress = address;
}

void receive(Common::Vector<uint8_t> data, MediaAccessControlAddress ourMac)
{
    auto* rawDataPtr = data.data();
    auto* frame = reinterpret_cast<Ethernet::FrameHeader*>(rawDataPtr);

    if (frame->macDest.get_value() == ourMac.get_value()) {
        switch (frame->ethertype.get_value()) {
        case EtherTypeValues::IPV4.get_value():
            InternetProtocolV4::receive(rawDataPtr + sizeof(FrameHeader), data.size() - sizeof(FrameHeader));
            break;
        case EtherTypeValues::ARP.get_value():
            AddressResolutionProtocol::receive(rawDataPtr + sizeof(FrameHeader), data.size() - sizeof(FrameHeader));
            break;
        }
    }
}

void send(MediaAccessControlAddress macDest, EtherType ethertype, uint8_t *buf, std::size_t size)
{
    size += sizeof(FrameHeader);
    void* newBuf = static_cast<uint8_t*>(kmalloc(size, 0));
    auto* frame = static_cast<FrameHeader*>(newBuf);
    frame->macDest = macDest;
    frame->macSource = macAddress;
    frame->ethertype = ethertype;

    memcpy(reinterpret_cast<uint8_t*>(newBuf) + sizeof(FrameHeader), buf, size - sizeof(FrameHeader));

    EthernetServer::instance().send_to_driver(static_cast<uint8_t*>(newBuf), size);

    /// FIXME: we should sent the buffer, not a pointer, to the driver so that it can be deallocated.
    //kfree(newBuf);
}

}
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

#include "Networking/IP/IPv4/Receive.h"
#include "Networking/IP/IPv4/Send.h"
#include "Networking/Ethernet/Send.h"
#include "Networking/ARP/Send.h"
#include "Networking/TCP/Socket.h"
#include "Memory/KernelHeap.h"

namespace Networking::InternetProtocolV4
{

InternetProtocolAddress ipAddress;
InternetProtocolAddress gatewayAddress;
std::uint32_t subnet;

void initialise(InternetProtocolAddress address, InternetProtocolAddress gateway, std::uint32_t subnetMask)
{
    ipAddress = address;
    gatewayAddress = gateway;
    subnet = subnetMask;
}

void receive(uint8_t* buf, std::size_t size)
{
    if (size < sizeof(MessageHeader))
        return;
    
    auto* message = reinterpret_cast<MessageHeader*>(buf);

    if (message->get_dest_ip() != ipAddress)
        return;
    
    if (message->get_length() > size)
        message->set_length(size);

    switch (message->get_protocol()) {
    case Protocol::TCP:
        TransmissionControlProtocol::Socket::receive_ip(message->get_dest_ip(), message->get_source_ip(), buf + 4 * message->get_ihl(), message->get_length() - 4 * message->get_ihl());
        break;
    case Protocol::UDP:
        break;
    }
}

void send(InternetProtocolAddress destIp, uint8_t protocol, uint8_t* buf, std::size_t size)
{
    size += sizeof(MessageHeader);
    auto* messageBuffer = reinterpret_cast<uint8_t*>(kmalloc(size, 0));
    auto* message = reinterpret_cast<MessageHeader*>(messageBuffer);

    message->set_version(Version::IPV4);
    message->set_ihl(sizeof(MessageHeader) / 4);
    message->set_length(size);
    message->set_ident(0x0001);
    message->set_flags(Flags::NO_FRAGMENT);
    message->set_fragment_offset(0);
    message->set_time_to_live(0x40);
    message->set_protocol(protocol);
    message->set_dest_ip(destIp);
    message->set_source_ip(ipAddress);
    message->set_header_checksum(0);
    auto chksum = Utilities::BigEndian<uint16_t>();
    chksum.set_raw_value(checksum(reinterpret_cast<uint16_t*>(message), sizeof(MessageHeader)));
    message->set_header_checksum(chksum.get_value());

    memcpy(messageBuffer + sizeof(MessageHeader), buf, size - sizeof(MessageHeader));

    InternetProtocolAddress route = destIp;

    if ((destIp.get_value() & subnet) != (message->get_source_ip().get_value() & subnet))
        route = gatewayAddress;
    auto routePhysicalAddress = AddressResolutionProtocol::request_and_wait_for_reply(route);

    Ethernet::send(routePhysicalAddress, Ethernet::EtherTypeValues::IPV4, messageBuffer, size);

    //kfree(buf);   
}

uint16_t checksum(uint16_t* data, std::size_t size)
{
    uint32_t temp = 0;

    for(std::size_t i = 0; i < size/2; i++)
        temp += ((data[i] & 0xFF00) >> 8) | ((data[i] & 0x00FF) << 8);

    if(size % 2)
        temp += ((uint16_t)((char*)data)[size-1]) << 8;
    
    while(temp & 0xFFFF0000)
        temp = (temp & 0xFFFF) + (temp >> 16);
    
    return ((~temp & 0xFF00) >> 8) | ((~temp & 0x00FF) << 8);
}

}
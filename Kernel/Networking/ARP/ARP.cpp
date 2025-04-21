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

#include "Networking/ARP/Receive.h"
#include "Networking/ARP/Send.h"
#include "Networking/Ethernet/Send.h"
#include "Networking/IP/IPv4/Common.h"

namespace Networking::AddressResolutionProtocol
{

namespace
{
    static inline AddressTable* m_table;
    static inline Mutex m_tableMutex;
    static inline Task::WaitQueue m_tableWaitQueue;
}

void initialise()
{
    m_table = new AddressTable();
}

void handle_request(Message* message)
{
    Message response(*message);
    response.operation = OperationValues::REPLY;
    response.targetHardwareAddress = message->senderHardwareAddress;
    response.targetProtocolAddress = message->senderProtocolAddress;
    response.senderHardwareAddress = Ethernet::macAddress;
    response.senderProtocolAddress = InternetProtocolV4::ipAddress;
    Ethernet::send(message->senderHardwareAddress, Ethernet::EtherTypeValues::ARP, reinterpret_cast<uint8_t*>(&response), sizeof(Message));
}

void handle_reply(Message* message)
{
    {
        LockAcquirer l(m_tableMutex);
        m_table->insert({message->senderProtocolAddress, message->senderHardwareAddress});
    }
    m_tableWaitQueue.wake_queue();
}

void receive(uint8_t* payload, std::size_t size)
{
    if (size < sizeof(Message))
        return;
    
    auto arpMsg = reinterpret_cast<Message*>(payload);
    if (arpMsg->hardwareType == HardwareTypeValues::ETHERNET
        && arpMsg->protocolType == Ethernet::EtherTypeValues::IPV4
        && arpMsg->hardwareAddressLen == HardwareAddressLengthValues::ETHERNET
        && arpMsg->protocolAddressLen == ProtocolAddressLengthValues::IPV4
        && arpMsg->targetProtocolAddress == InternetProtocolV4::ipAddress
    ) {
        switch (arpMsg->operation.get_value())
        {
            case OperationValues::REQUEST.get_value():
            handle_request(arpMsg);
            break;
            case OperationValues::REPLY.get_value():
            handle_reply(arpMsg);
            break;
        }
    }
}

void send_request(InternetProtocolAddress reqIp)
{
    Message message = {
        .hardwareType = HardwareTypeValues::ETHERNET,
        .protocolType = Ethernet::EtherTypeValues::IPV4,
        .hardwareAddressLen = HardwareAddressLengthValues::ETHERNET,
        .protocolAddressLen = ProtocolAddressLengthValues::IPV4,
        .operation = OperationValues::REQUEST,
        .senderHardwareAddress = Ethernet::macAddress,
        .senderProtocolAddress = InternetProtocolV4::ipAddress,
        .targetHardwareAddress = 0,
        .targetProtocolAddress = reqIp
    };
    
    Ethernet::send(BROADCAST_MAC, Ethernet::EtherTypeValues::ARP, reinterpret_cast<uint8_t*>(&message), sizeof(Message));
}

Common::Optional<MediaAccessControlAddress> find_cached_address(InternetProtocolAddress ipAddress)
{
    LockAcquirer l(m_tableMutex);
    auto it = m_table->find(ipAddress);
    if (it != m_table->end())
        return it->second;
    return Common::Nullopt;
}

MediaAccessControlAddress request_and_wait_for_reply(InternetProtocolAddress reqIp)
{
    Common::Optional<MediaAccessControlAddress> macAddress = find_cached_address(reqIp); 
    
    auto wqItem = m_tableWaitQueue.add_to_queue();
    if (macAddress.has_value())
        goto done;
    send_request(reqIp);
    
    do {
        Task::Manager::instance().block();
        macAddress = find_cached_address(reqIp);
    } while (!macAddress.has_value());
done:
    m_tableWaitQueue.remove_from_queue(wqItem);
    return *macAddress;
}

}
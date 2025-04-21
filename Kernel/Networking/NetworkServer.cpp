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

#include "Common/Vector.h"
#include "Networking/NetworkServer.h"
#include "Networking/Ethernet/Receive.h"

namespace Networking
{

void EthernetServer::start()
{
    for (int i = 0; i < THREADPOOL_COUNT; i++) {
        Task::Manager::instance().launch_thread(reinterpret_cast<void*>(&network_thread), this);
    }
    m_started = true;
    Task::Manager::instance().about_to_block();
    Task::Manager::instance().block();
    //IPC::Server<NetworkServerEndpoint> messageServer("net::msgserver");
    //messageServer.start<EthernetServer>();
}

bool EthernetServer::has_started()
{
    return m_started;
}

/*void EthernetServer::handle_ipc_message(RegisterNetworkDevice registration)
{
    instance().m_networkDriver = reinterpret_cast<Drivers::EthernetNetworkDriver*>(reinterpret_cast<void*>(registration.pointer));
}*/

void EthernetServer::register_device(Drivers::EthernetNetworkDriver* driver)
{
    m_networkDriver = driver;
}

void EthernetServer::send_to_driver(uint8_t* buf, std::size_t size)
{
    instance().m_networkDriver->send_data((buf), size);
}

void EthernetServer::receive_from_driver(Common::Vector<uint8_t> data)
{
    {
        LockAcquirer l(instance().m_messageQueueLock);
        instance().m_messageQueue.push_back(std::move(data));
    }
    instance().m_threadWaitQueue.wake_one();
}

/*void EthernetServer::handle_ipc_message(NetworkDeviceMessage message)
{
    {
        LockAcquirer l(instance().m_messageQueueLock);
        instance().m_messageQueue.push_back(std::move(message));
    }
    instance().m_threadWaitQueue.wake_one();
}*/

void EthernetServer::network_thread(EthernetServer* server)
{
    while (true) {
        auto wqItem = server->m_threadWaitQueue.add_to_queue();

        server->m_messageQueueLock.acquire();

        while (server->m_messageQueue.size() == 0) {
            server->m_messageQueueLock.release();
            Task::Manager::instance().block();
            server->m_messageQueueLock.acquire();
        }
        server->m_threadWaitQueue.remove_from_queue(wqItem);

        auto messageIt = server->m_messageQueue.begin();
        auto message = std::move(*messageIt);
        
        server->m_messageQueue.erase(messageIt);

        server->m_messageQueueLock.release();
        auto mac = server->m_networkDriver->get_mac_address();
        Networking::Ethernet::receive(std::move(message), mac);
        //Networking::Ethernet::EthernetLayerManager::instance()->on_receive_data(message.data.data(), message.data.size());
    }
}


}
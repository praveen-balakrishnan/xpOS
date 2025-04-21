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

#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include "Common/Vector.h"
#include "Drivers/Network/NetworkDriver.h"
#include "Memory/KernelHeap.h"
#include "Tasks/Mutex.h"
#include "Tasks/WaitQueue.h"

namespace Networking
{

/*struct RegisterNetworkDevice
{
    static constexpr int MessageId = 1;
    std::uintptr_t pointer;

    template<typename Archive>
    void define_archivable(Archive& ar)
    {
        ar(pointer);
    }
};

struct NetworkDeviceMessage
{
    static constexpr int MessageId = 2;
    Common::Vector<uint8_t> data;

    template<typename Archive>
    void define_archivable(Archive& ar)
    {
        ar(data);
    }
};

using NetworkServerEndpoint = IPC::Endpoint<RegisterNetworkDevice, NetworkDeviceMessage>;*/

class EthernetServer
{
public:
    void start();
    
    static EthernetServer& instance()
    {
        static EthernetServer instance;
        return instance;
    }

    void register_device(Drivers::EthernetNetworkDriver* ptr);
    void send_to_driver(uint8_t* buf, std::size_t size);
    void receive_from_driver(Common::Vector<uint8_t> data);
    bool has_started();
private:
    EthernetServer() = default;
    Task::WaitQueue m_threadWaitQueue;
    Common::List<Common::Vector<uint8_t>> m_messageQueue;
    Mutex m_messageQueueLock;
    Drivers::EthernetNetworkDriver* m_networkDriver;
    static void network_thread(EthernetServer* server);
    static constexpr int THREADPOOL_COUNT = 4;
    bool m_started = false;
};

}

#endif
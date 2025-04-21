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

#ifndef NETWORKSOCKET_H
#define NETWORKSOCKET_H

#include "Pipes/Pipe.h"
#include "TCP/Socket.h"

namespace Networking
{
class NetworkSocket
{
public:
    static bool connect(Pipes::Pipe& pipe, Endpoint endpoint);
    static bool bind(Pipes::Pipe& pipe, Endpoint endpoint);
    static bool listen(Pipes::Pipe& pipe);
    static bool accept(Pipes::Pipe& pipe, Pipes::Pipe& newPipe);
    static void initialise();
    void receive(uint8_t* data, uint32_t size);
    void conn_request();

    enum class Flags : int
    {
        NON_BLOCKING = 1
    };
    
    enum EventTypes : Pipes::EventType
    {
        SOCK_REQUEST_CONN = 0x4
    };

private:
    NetworkSocket(int flags, uint8_t protocol)
                : m_shouldNotBlock(flags & static_cast<int>(Flags::NON_BLOCKING))
                , m_protocol(protocol)
            {}
    static bool open(void* with, int flags, void*& deviceSpecific);
    static void close(void*& deviceSpecific);
    static std::size_t read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific);
    static std::size_t write(std::size_t offset, std::size_t count, const void* buf, void*& deviceSpecific);
    static Pipes::EventListenerList::Receipt notify(void* listener, Pipes::raise_events_callback raise_event, Pipes::EventTypeMask& current, void*& deviceSpecific);
    static void denotify(Pipes::EventListenerList::Receipt receipt, void*& deviceSpecific);

    static constexpr int QUEUE_SIZE = 1024;

    Mutex m_lock;

    Common::FixedCircularBuffer<QUEUE_SIZE> m_queue;

    //bool m_binded = false;
    //bool m_connected = false;
    //bool m_listening = false;
    bool m_shouldNotBlock = true;
    Task::TaskID m_tid = 0;
    int m_connections = 0;

    uint8_t m_protocol;

    void* m_protocolSock;

    Task::WaitQueue m_readWaitQueue;
    //Task::WaitQueue m_writeWaitQueue;
    Task::WaitQueue m_connectWaitQueue;

    Pipes::EventListenerList m_eventListenerList;

    static inline Pipes::DeviceOperations m_deviceOperations;
};
}

#endif
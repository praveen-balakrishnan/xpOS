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

#ifndef SOCKET_H
#define SOCKET_H

#include "Common/CircularBuffer.h"
#include "Common/Hashmap.h"
#include "Common/Expected.h"
#include "Common/String.h"
#include "Pipes/EventType.h"
#include "Pipes/Pipe.h"
#include "Tasks/Mutex.h"
#include "Tasks/TaskManager.h"
#include "Tasks/WaitQueue.h"

namespace Sockets
{

class GenericSocket
{
public:
    static bool connect(Pipes::Pipe& pipe, const char* id);
    static bool bind(Pipes::Pipe& pipe, const char* id);
    static bool listen(Pipes::Pipe& pipe);
    static bool accept(Pipes::Pipe& pipe, Pipes::Pipe& newPipe);
    static void initialise();
    enum class Flags : int
    {
        NON_BLOCKING = 1
    };

    enum EventTypes : Pipes::EventType
    {
        SOCK_REQUEST_CONN = 0x4
    };

private:
    GenericSocket(int flags)
        : m_shouldNotBlock(flags & static_cast<int>(Flags::NON_BLOCKING))
    {}
    static bool open(void* with, int flags, void*& deviceSpecific);
    static void close(void*& deviceSpecific);
    static std::size_t read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific);
    static std::size_t write(std::size_t offset, std::size_t count, const void* buf, void*& deviceSpecific);
    static Pipes::EventListenerList::Receipt notify(void* listener, Pipes::raise_events_callback raise_event, Pipes::EventTypeMask& current, void*& deviceSpecific);
    static void denotify(Pipes::EventListenerList::Receipt receipt, void*& deviceSpecific);

    static constexpr int QUEUE_SIZE = 1024;

    static Common::Hashmap<Common::HashableString, LocalSocket*>& bind_map()
    {
        static Common::Hashmap<Common::HashableString, LocalSocket*> bindMap;
        return bindMap;
    }

    inline static Mutex m_mapLock;

    Mutex m_lock;

    char m_identifier[256];

    Common::FixedCircularBuffer<QUEUE_SIZE> m_queue;

    LocalSocket* m_endpoint = nullptr;

    bool m_binded = false;
    bool m_connected = false;
    bool m_listening = false;
    bool m_shouldNotBlock = true;
    Task::TaskID m_tid = 0;

    Task::WaitQueue m_readWaitQueue;
    Task::WaitQueue m_writeWaitQueue;
    Task::WaitQueue m_connectWaitQueue;

    Common::List<LocalSocket*> m_connectionQueue;

    Pipes::EventListenerList m_eventListenerList;

    static inline Pipes::DeviceOperations m_deviceOperations;
};

}

#endif
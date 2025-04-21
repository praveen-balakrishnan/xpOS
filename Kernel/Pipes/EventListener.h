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

#ifndef EVENTLISTENER_H
#define EVENTLISTENER_H

#include "API/Event.h"
#include "Common/Hashmap.h"
#include "Pipes/Pipe.h"
#include "Tasks/Mutex.h"
#include "Tasks/WaitQueue.h"
#include <cstdint>

namespace Pipes
{
using namespace xpOS;

/**
 * An EventListener can be used to monitor events on multiple pipes 
 * concurrently in an efficient manner. 
 */
class EventListener
{
    static void inform(void* listener, EventTypeMask mask, uint64_t pipe);

    void inform(EventTypeMask mask, uint64_t pipe);

    void wake();

    static bool open(void* with, int flags, void*& deviceSpecific);
    static void close(void*& deviceSpecific);
    static std::size_t read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific);
    int listen(API::Event* eventListResult, int maxEvents);

public:
    static void initialise();
    static void add(Pipe& listener, Pipe* pipe, uint64_t pd, EventTypeMask eventMask);

    static void remove(Pipe& listener, Pipe* pipe, uint64_t pd, EventTypeMask event);

private:
    EventListener() {}
    ~EventListener();
    Common::Hashmap<std::pair<Pipe*, uint64_t>, PipeNotifReg> m_eventNotifRegs;
    Mutex m_eventNotifRegLock;

    Mutex m_taskQueuesLock;
    Task::WaitQueue m_waitQueue;
    Common::List<std::pair<API::Event*, int>> m_resultPtrQueue;
    Common::List<API::Event> m_raisedEvents;

    static inline Pipes::DeviceOperations m_deviceOperations;
};

}
#endif
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

#include "Pipes/EventListener.h"
#include "Common/Vector.h"

namespace Pipes
{

void EventListener::initialise()
{
    m_deviceOperations = {
        .open = EventListener::open,
        .close = EventListener::close,
        .read = EventListener::read
    };
    Pipes::register_device("elistener", &m_deviceOperations);
}

bool EventListener::open(void* with, int flags, void*& deviceSpecific)
{
    deviceSpecific = new EventListener();
    return true;
}

void EventListener::close(void*& deviceSpecific)
{
    auto* listener = static_cast<EventListener*>(deviceSpecific);
    delete listener;
}

void EventListener::inform(void* listener, EventTypeMask mask, uint64_t pipe)
{
    static_cast<EventListener*>(listener)->inform(mask, pipe);
}

void EventListener::inform(EventTypeMask mask, uint64_t pipe)
{
    LockAcquirer a(m_taskQueuesLock);
    bool updatedRaisedEvent = false;
    // If we already have events on a given pipe, just add the new raised
    // events to it.
    for (auto& [oldMask, ipipe] : m_raisedEvents) {
        if (ipipe == pipe) {
            oldMask |= mask;
            updatedRaisedEvent = true;
            break;
        }
    }

    if (!updatedRaisedEvent)
        m_raisedEvents.push_back({mask, pipe});

    /*if (m_waitQueue.empty())
        return;
    
    for (auto [eventList, max] : m_resultPtrQueue) {
        int i = 0;
        for (auto& event : m_raisedEvents) {
            if (i == max)
                break;
            eventList[i] = event;
            i++;
        }
    }

    m_waitQueue.wake_queue();

    m_raisedEvents.clear();*/
    wake();
}

std::size_t EventListener::read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific)
{
    int maxEvents = count / sizeof(API::Event);
    auto listener = reinterpret_cast<EventListener*>(deviceSpecific);
    API::Event* events = new API::Event[count];

    auto raised = listener->listen(events, maxEvents) * sizeof(API::Event);
    memcpy(buf, events, raised);
    delete[] events;
    return raised;
}

void EventListener::wake()
{
    if (m_waitQueue.empty())
        return;
    for (auto [eventList, max] : m_resultPtrQueue) {
        int i = 0;
        for (auto& event : m_raisedEvents) {
            if (i == max)
                break;
            eventList[i] = event;
            i++;
        }
    }

    m_waitQueue.wake_queue();
}

void EventListener::add(Pipe& listener, Pipe* pipe, uint64_t pd, EventTypeMask eventMask)
{
    auto* l = static_cast<EventListener*>(listener.device_specific());
    LockAcquirer acq(l->m_eventNotifRegLock);
    
    EventTypeMask currentEvents;

    PipeNotifReg pipeNotifReg = pipe->notify(eventMask, l, inform, pd, currentEvents);
    l->m_eventNotifRegs.insert(std::pair(std::pair(pipe, eventMask), pipeNotifReg));
    if (currentEvents & eventMask)
        l->inform(currentEvents & eventMask, pd);
}

void EventListener::remove(Pipe& listener, Pipe* pipe, uint64_t pd, EventTypeMask event)
{
    auto* l = static_cast<EventListener*>(listener.device_specific());
    LockAcquirer acq(l->m_eventNotifRegLock);
    auto pipeNotifReg = l->m_eventNotifRegs.find({pipe, event});
    if (pipeNotifReg == l->m_eventNotifRegs.end())
        return;
    
    pipe->unnotify(pipeNotifReg->second);
}

int EventListener::listen(API::Event* eventListResult, int maxEvents)
{
    bool eventQueueEmpty = false;
    int eventsReceived;
    Task::WaitQueue::Item wqItem;
    
    auto it = m_resultPtrQueue.end();
    {
        LockAcquirer a(m_taskQueuesLock);
        wqItem = m_waitQueue.add_to_queue();
        m_resultPtrQueue.push_back({eventListResult, maxEvents});
        it = --m_resultPtrQueue.end();
        eventQueueEmpty = m_raisedEvents.size() == 0;
    }

    if (eventQueueEmpty) {
        Task::Manager::instance().block();
        m_taskQueuesLock.acquire();
    } else {
        m_taskQueuesLock.acquire();
        wake();
    }
    eventsReceived = m_raisedEvents.size();
    m_raisedEvents.clear();
    m_resultPtrQueue.erase(it);
    m_waitQueue.remove_from_queue(wqItem);
    m_taskQueuesLock.release();

    return eventsReceived;
}

EventListener::~EventListener()
{
    for (auto& [pipeWrapped, reg] : m_eventNotifRegs)
        pipeWrapped.first->unnotify(reg);
}

}
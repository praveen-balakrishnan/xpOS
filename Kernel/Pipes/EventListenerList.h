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

#ifndef EVENTLISTENERLIST_H
#define EVENTLISTENERLIST_H

#include "Tasks/Mutex.h"
#include "Pipes/EventType.h"

namespace Pipes
{

class EventListenerList
{
    using ListenerQueue = Common::List<std::pair<void*, raise_events_callback>>;

public:
    using Receipt = ListenerQueue::Iterator;

    Receipt add(void* listener, raise_events_callback callback)
    {
        LockAcquirer a(m_lock);
        if (!listener)
            return m_listeners.end();
        
        m_listeners.push_back({listener, callback});
        return --m_listeners.end();
    }

    void notify(EventTypeMask events)
    {
        LockAcquirer a(m_lock);
        for (auto& [listener, callback] : m_listeners)
            callback(listener, events);
    }

    void remove(Receipt receipt)
    {
        LockAcquirer a(m_lock);
        if (receipt == m_listeners.end())
            return;
        m_listeners.erase(receipt);
    }

private:
    Mutex m_lock;
    ListenerQueue m_listeners;
};

}

#endif
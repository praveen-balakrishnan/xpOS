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

#ifndef PIPE_H
#define PIPE_H

#include <cstdint>

#include "API/Pipes.h"
#include "Pipes/EventType.h"
#include "Pipes/EventListenerList.h"
#include "Common/List.h"
#include "Common/Optional.h"
#include "Tasks/Mutex.h"

namespace Pipes
{
class Pipe;

typedef void (*wake_listener_callback)(void* listener, EventTypeMask mask, uint64_t pd);

typedef bool (*open_callback)(void* with, int flags, void*& deviceSpecific); 
typedef void (*close_callback)(void*& deviceSpecific);
typedef std::size_t (*read_callback)(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific);
typedef std::size_t (*write_callback)(std::size_t offset, std::size_t count, const void* buf, void*& deviceSpecific);
typedef EventListenerList::Receipt (*notify_callback)(void* listener, raise_events_callback raise_event, EventTypeMask& current, void*& deviceSpecific);
typedef void (*denotify_callback)(EventListenerList::Receipt, void*& deviceSpecific);
typedef xpOS::API::Pipes::PipeInfo (*info_callback)(void*& deviceSpecific);

struct Listener
{
    EventTypeMask interestedEvents;
    void* listener;
    wake_listener_callback callback;
    uint64_t pd;
};

using PipeNotifReg = Common::List<Listener>::Iterator;

/**
 * A set of operations an endpoint device can perform.
 * 
 * The operations must be set to nullptr if not implemented, so they can be defaulted.
 */
struct DeviceOperations
{
    open_callback open;
    close_callback close;
    read_callback read;
    write_callback write;
    notify_callback notify;
    denotify_callback denotify;
    info_callback info;
};

void register_device(const char* device, DeviceOperations* ops);
void initialise();


/**
 * Pipes represent an abstraction of different devices and objects that can be
 * read and written to (as well as other device-specific operations).
 */
class Pipe
{
    friend class EventListener;
public:
    Pipe(const char* device, void* with = nullptr, int flags = 0);
    Pipe(const Pipe& pipe) = delete;
    Pipe& operator=(Pipe pipe) = delete;
    Pipe(Pipe&& pipe);
    Pipe& operator=(Pipe&& pipe);
    ~Pipe();

    friend void swap(Pipe& a, Pipe& b);
    std::size_t read(std::size_t count, void* buf);
    std::size_t write(std::size_t count, const void* buf);
    std::size_t seek(long count, xpOS::API::Pipes::SeekType type);
    xpOS::API::Pipes::PipeInfo info();

    bool has_active_connection() 
    {
        return m_isOpen;
    }

    void*& device_specific()
    {
        return m_deviceSpecific;
    }

private:
    void* m_deviceSpecific;
    bool m_isOpen = false;
    DeviceOperations* m_device;
    std::size_t m_offset = 0;

    // The notify and unnotify operations are used by EventListeners to
    // register interest in events.
    PipeNotifReg notify(EventTypeMask event, void* listener, wake_listener_callback callback, uint64_t pipe, EventTypeMask& currentEvents);
    void unnotify(PipeNotifReg notifReg);

    static void raise_event(void* listener, EventTypeMask event);

    Common::List<Listener> m_listeners;
    Common::Optional<EventListenerList::Receipt> m_devReceipt;
    Mutex m_notifyLock;
};

}

#endif
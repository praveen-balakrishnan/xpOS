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

#include "Common/Hashmap.h"
#include "Common/String.h"
#include "Pipes/Pipe.h"
#include "Tasks/Spinlock.h"

namespace Pipes
{

namespace
{
    static Common::Hashmap<Common::HashableString, DeviceOperations*>* pipeDevices;
    static Spinlock lock;
}

void initialise()
{
    pipeDevices = new Common::Hashmap<Common::HashableString, DeviceOperations*>();
}

void register_device(const char* device, DeviceOperations* ops)
{
    LockAcquirer acquirer(lock);
    pipeDevices->insert({device, ops});
}

Pipe::Pipe(const char* device, void* with, int flags)
{
    DeviceOperations* operations;
    {
        LockAcquirer acquirer(lock);
        auto it = pipeDevices->find(device);
        if (it == pipeDevices->end())
            return;
        
        operations = it->second;
    }
    
    if (!operations->open)
        return;
    
    if (!operations->open(with, flags, m_deviceSpecific))
        return;
    
    m_isOpen = true;
    m_device = operations;
}

Pipe::~Pipe()
{
    if (has_active_connection() && m_device->close)
        m_device->close(m_deviceSpecific);
}

void swap(Pipe& a, Pipe& b)
{
    using std::swap;
    LockAcquirer la(a.m_notifyLock);
    LockAcquirer lb(b.m_notifyLock);

    swap(a.m_isOpen, b.m_isOpen);
    swap(a.m_device, b.m_device);
    swap(a.m_deviceSpecific, b.m_deviceSpecific);
    swap(a.m_offset, b.m_offset);
    swap(a.m_listeners, b.m_listeners);
    swap(a.m_devReceipt, b.m_devReceipt);
}

Pipe::Pipe(Pipe&& pipe)
    : m_isOpen(false)
{
    swap(pipe, *this);
}

Pipe& Pipe::operator=(Pipe&& pipe)
{
    swap(pipe, *this);
    return *this;
}

std::size_t Pipe::read(std::size_t count, void* buf)
{
    if (!has_active_connection() || !m_device->read)
        return 0;
    
    auto readCount = m_device->read(m_offset, count, buf, m_deviceSpecific);
    seek(readCount, xpOS::API::Pipes::SeekType::CUR);
    return readCount;
}

std::size_t Pipe::write(std::size_t count, const void* buf)
{
    if (!has_active_connection() || !m_device->write)
        return 0;
    
    auto writeCount = m_device->write(m_offset, count, buf, m_deviceSpecific);
    seek(writeCount, xpOS::API::Pipes::SeekType::CUR);
    return writeCount;
}

std::size_t Pipe::seek(long count, xpOS::API::Pipes::SeekType type)
{
    using T = xpOS::API::Pipes::SeekType;
    switch (type) {
    case T::CUR:
        m_offset += count;
        break;
    case T::SET:
        m_offset = count;
        break;
    case T::END:
        break;
    }
    return m_offset;
}

xpOS::API::Pipes::PipeInfo Pipe::info()
{
    if (!has_active_connection() || !m_device->write)
        return {};
    
    return m_device->info(m_deviceSpecific);
}

void Pipe::raise_event(void* listener, EventTypeMask event)
{
    auto* pipe = static_cast<Pipe*>(listener);
    LockAcquirer a(pipe->m_notifyLock);
    for (auto& l : pipe->m_listeners) {
        if (l.interestedEvents & event) {
            l.callback(l.listener, l.interestedEvents & event, l.pd);
        }
    }
}

PipeNotifReg Pipe::notify(EventTypeMask eventMask, void* listener, wake_listener_callback callback, uint64_t pd, EventTypeMask& currentMask)
{
    LockAcquirer a(m_notifyLock);

    if (!m_isOpen)
        return m_listeners.end();
    
    EventTypeMask maskOut = 0;
    if (!m_device->notify)
        return m_listeners.end();
    
    Listener l {
        .interestedEvents = eventMask,
        .listener = listener,
        .callback = callback,
        .pd = pd
    };

    m_listeners.push_back(l);
    
    if (m_devReceipt.has_value()) {
        m_device->notify(nullptr, nullptr, maskOut, m_deviceSpecific);
    } else {
        m_devReceipt = m_device->notify(this, raise_event, maskOut, m_deviceSpecific);
    }
    currentMask = maskOut;

    return --m_listeners.end();
}

void Pipe::unnotify(PipeNotifReg notifReg)
{
    LockAcquirer a(m_notifyLock);

    m_listeners.erase(notifReg);

    if (!m_device->denotify)
        return;
    
    if (!m_devReceipt.has_value())
        return;
    
    if (m_listeners.size() == 0) {
        m_device->denotify(*m_devReceipt, m_deviceSpecific);
    }
}

}
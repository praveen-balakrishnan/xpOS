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

#include "Pipes/LocalSocket.h"

namespace Sockets
{

void LocalSocket::initialise()
{
    m_deviceOperations = {
        .open = LocalSocket::open,
        .close = LocalSocket::close,
        .read = LocalSocket::read,
        .write = LocalSocket::write,
        .notify = LocalSocket::notify,
        .denotify = LocalSocket::denotify
    };
    Pipes::register_device("socket::local", &m_deviceOperations);
}

bool LocalSocket::open(void* with, int flags, void*& deviceSpecific)
{
    deviceSpecific = new LocalSocket(flags);
    return true;
}

/// FIXME: properly close and wake read/write queues so they do not indefinitely sleep
void LocalSocket::close(void*& deviceSpecific)
{
    auto* socket = static_cast<LocalSocket*>(deviceSpecific);
    {
        LockAcquirer acquirer(m_mapLock);
        bind_map().erase(socket->m_identifier);
    }
    deviceSpecific = nullptr;
    
    delete socket;
}

bool LocalSocket::bind(Pipes::Pipe& pipe, const char* id)
{
    auto* socket = static_cast<LocalSocket*>(pipe.device_specific());

    if (socket->m_binded)
        return false;
    
    {
        LockAcquirer acquirer(m_mapLock);
        if (bind_map().find(id) != bind_map().end())
            return false;
        
        strcpy(socket->m_identifier, id);
        bind_map().insert({id, socket});
    }
    
    socket->m_binded = true;
    return true;
}

bool LocalSocket::listen(Pipes::Pipe& pipe)
{
    auto* socket = static_cast<LocalSocket*>(pipe.device_specific());

    if (!socket->m_binded)
        return false;
    
    socket->m_listening = true;
    return true;
}

bool LocalSocket::accept(Pipes::Pipe& pipe, Pipes::Pipe& newPipe)
{
    auto* socket = static_cast<LocalSocket*>(pipe.device_specific());

    bool shouldBlock;
    {
        LockAcquirer acquirer(socket->m_lock);

        if (!socket->m_listening)
            return false;
        
        auto first = socket->m_connectionQueue.begin();
        shouldBlock = first == socket->m_connectionQueue.end() && !socket->m_shouldNotBlock;
    }

    if (shouldBlock) {
        {
            LockAcquirer acquirer(socket->m_lock);
            socket->m_tid = Task::Manager::instance().get_current_tid();
            Task::Manager::instance().about_to_block();
        }
        Task::Manager::instance().block();
    }

    LocalSocket* remoteSocket;

    {
        LockAcquirer acquirer(socket->m_lock);

        auto first = socket->m_connectionQueue.begin();
        if (first != socket->m_connectionQueue.end()) {
            remoteSocket = *first;
            socket->m_connectionQueue.erase(first);
        } else {
            return false;
        }
    }
    
    LockAcquirer acquirer(socket->m_lock);
    
    auto* newSocket = static_cast<LocalSocket*>(newPipe.device_specific());

    newSocket->m_endpoint = remoteSocket;
    remoteSocket->m_endpoint = newSocket;
    newSocket->m_connected = true;

    Task::Manager::instance().unblock(remoteSocket->m_tid);
    return true;
}

bool LocalSocket::connect(Pipes::Pipe& pipe, const char* id)
{
    auto* socket = static_cast<LocalSocket*>(pipe.device_specific());
    socket->m_tid = Task::Manager::instance().get_current_tid();

    LocalSocket* remoteSocket;
    {
        LockAcquirer acquirer(m_mapLock);
        auto it = bind_map().find(id);
        if (it == bind_map().end())
            return false;
        
        remoteSocket = it->second;
    }

    /// FIXME: This should not block on a non-blocking socket.

    Task::TaskID remoteTid;
    
    {
        LockAcquirer acquirer(remoteSocket->m_lock);
        if (!remoteSocket->m_listening)
            return false;
        
        remoteSocket->m_connectionQueue.push_back(socket);
        remoteSocket->m_eventListenerList.notify(LocalSocket::EventTypes::SOCK_REQUEST_CONN);
        remoteTid = remoteSocket->m_tid;
    }
    
    socket->m_connected = true;
    
    Task::Manager::instance().about_to_block();
    if (remoteTid)
        Task::Manager::instance().unblock(remoteSocket->m_tid);

    Task::Manager::instance().block();
    return true;
}

std::size_t LocalSocket::read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific)
{
    auto* socket = static_cast<LocalSocket*>(deviceSpecific);
    LockAcquirer acquire(socket->m_lock);

    Task::WaitQueue::Item wqitem = socket->m_readWaitQueue.add_to_queue();

    while (socket->m_connected && socket->m_queue.is_queue_empty()) { 
        if (socket->m_shouldNotBlock)
            return 0;   
        socket->m_lock.release();
        Task::Manager::instance().block();
        socket->m_lock.acquire();
    }
    
    socket->m_readWaitQueue.remove_from_queue(wqitem);

    if (!socket->m_connected)
        return 0;

    //std::size_t readCount = 0;
    auto socketWasFull = socket->m_queue.is_queue_full();

    /*auto start = socket->m_queueFront;

    auto socketWasFull = socket->is_queue_full();

    while (!socket->is_queue_empty() && count--) {
        socket->m_queueFront += 1;
        socket->m_queueFront %= QUEUE_SIZE;
        readCount++;
    }
    
    auto end = socket->m_queueFront;

    if (start <= end) {
        memcpy(buf, socket->m_queue + start, end - start);
    } else { 
        memcpy(buf, socket->m_queue + start, QUEUE_SIZE - start);
        memcpy(static_cast<char*>(buf) + (QUEUE_SIZE - start), socket->m_queue, end);
    }*/

   auto readCount = socket->m_queue.read(count, buf);

    if (readCount > 0) {
        socket->m_writeWaitQueue.wake_queue();
        if (socketWasFull)
            socket->m_eventListenerList.notify(Pipes::EventTypes::WRITEABLE);
    }
    return readCount;
}

std::size_t LocalSocket::write(std::size_t offset, std::size_t count, const void* buf, void*& deviceSpecific)
{
    auto* socket = static_cast<LocalSocket*>(deviceSpecific);
    LocalSocket* remoteSocket;
    bool shouldNotBlock;

    {
        LockAcquirer acquire(socket->m_lock);
        remoteSocket = socket->m_endpoint;
        shouldNotBlock = socket->m_shouldNotBlock;
    }

    if (!remoteSocket)
        return 0;
    
    LockAcquirer acquire(remoteSocket->m_lock);

    Task::WaitQueue::Item wqitem = remoteSocket->m_writeWaitQueue.add_to_queue();

    while (remoteSocket->m_connected && remoteSocket->m_queue.is_queue_full()) {
        if (shouldNotBlock)
            return 0;    
        remoteSocket->m_lock.release();
        Task::Manager::instance().block();
        remoteSocket->m_lock.acquire();
    }
    remoteSocket->m_writeWaitQueue.remove_from_queue(wqitem);

    if (!remoteSocket->m_connected)
        return 0;
    auto socketWasEmpty = remoteSocket->m_queue.is_queue_empty();

    /*std::size_t writeCount = 0;
    auto start = remoteSocket->m_queueBack;

    while (!remoteSocket->is_queue_full() && count--) {
        remoteSocket->m_queueBack += 1;
        remoteSocket->m_queueBack %= QUEUE_SIZE;
        writeCount++;
    }

    auto end = remoteSocket->m_queueBack;

    if (start <= end) {
        memcpy(remoteSocket->m_queue + start, buf, end - start);
    } else {
        memcpy(remoteSocket->m_queue + start, buf, QUEUE_SIZE - start);
        memcpy(remoteSocket->m_queue, static_cast<const char*>(buf) + (QUEUE_SIZE - start), end);
    }*/
    auto writeCount = remoteSocket->m_queue.write(count, buf);

    if (writeCount > 0) {
        remoteSocket->m_readWaitQueue.wake_queue();
        if (socketWasEmpty)
            remoteSocket->m_eventListenerList.notify(Pipes::EventTypes::READABLE);
    }
    return writeCount;
}

Pipes::EventListenerList::Receipt LocalSocket::notify(void* listener, Pipes::raise_events_callback raise_event, Pipes::EventTypeMask& current, void*& deviceSpecific)
{
    auto* socket = static_cast<LocalSocket*>(deviceSpecific);

    Pipes::EventTypeMask mask = 0;
    LocalSocket* remoteSocket;
    {
        LockAcquirer acquire(socket->m_lock);
        if (socket->m_connected && !socket->m_queue.is_queue_empty())
            mask |= Pipes::EventTypes::READABLE;
        
        if (socket->m_connectionQueue.size() > 0)
            mask |= LocalSocket::EventTypes::SOCK_REQUEST_CONN;
        remoteSocket = socket->m_endpoint;
    }

    if (remoteSocket) {
        LockAcquirer acquire(remoteSocket->m_lock);
        if (remoteSocket->m_connected && !remoteSocket->m_queue.is_queue_full())
            mask |= Pipes::EventTypes::WRITEABLE;
    }
    
    current = mask;

    LockAcquirer acquire(socket->m_lock);
    
    return socket->m_eventListenerList.add(listener, raise_event);
}

void LocalSocket::denotify(Pipes::EventListenerList::Receipt receipt, void*& deviceSpecific)
{
    auto* socket = static_cast<LocalSocket*>(deviceSpecific);
    LockAcquirer acquire(socket->m_lock);

    socket->m_eventListenerList.remove(receipt);
}

}
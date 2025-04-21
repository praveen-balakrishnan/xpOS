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

#include "Networking/NetworkSocket.h"
#include "Networking/TCP/Socket.h"

namespace Networking
{

void NetworkSocket::initialise()
{
    m_deviceOperations = {
        .open = NetworkSocket::open,
        .close = NetworkSocket::close,
        .read = NetworkSocket::read,
        .write = NetworkSocket::write,
        .notify = NetworkSocket::notify,
        .denotify = NetworkSocket::denotify
    };
    Pipes::register_device("socket::network", &m_deviceOperations);
}

bool NetworkSocket::open(void* with, int flags, void*& deviceSpecific)
{
    deviceSpecific = new NetworkSocket(flags, InternetProtocolV4::Protocol::TCP);
    return true;
}

void NetworkSocket::close(void*& deviceSpecific)
{
    delete reinterpret_cast<NetworkSocket*>(deviceSpecific);
}


bool NetworkSocket::bind(Pipes::Pipe& pipe, Endpoint endpoint)
{
    auto* netSock = static_cast<NetworkSocket*>(pipe.device_specific());
    netSock->m_protocolSock = TransmissionControlProtocol::Socket::create_socket(endpoint, netSock);
    return true;
}

void NetworkSocket::conn_request()
{
    LockAcquirer l(m_lock);
    m_connections++;
    m_connectWaitQueue.wake_one();
}

bool NetworkSocket::accept(Pipes::Pipe& pipe, Pipes::Pipe& newPipe)
{
    auto* socket = static_cast<NetworkSocket*>(pipe.device_specific());

    bool shouldBlock;
    
    {
        LockAcquirer acquirer(socket->m_lock);
        
        shouldBlock = socket->m_connections == 0 && !socket->m_shouldNotBlock;
    }

    if (shouldBlock) {
        {
            LockAcquirer acquirer(socket->m_lock);
            Task::Manager::instance().about_to_block();
            socket->m_connectWaitQueue.add_to_queue();
        }
        Task::Manager::instance().block();
    }

    {
        LockAcquirer acquirer(socket->m_lock);
        auto* newSocket = static_cast<NetworkSocket*>(newPipe.device_specific());
        auto* newTcpSock = reinterpret_cast<TransmissionControlProtocol::Socket*>(newSocket->m_protocolSock);
        reinterpret_cast<TransmissionControlProtocol::Socket*>(socket->m_protocolSock)->accept_conn_request(newTcpSock, socket);
    }
    
    socket->m_connections--;
    return true;
}

bool NetworkSocket::connect(Pipes::Pipe& pipe, Endpoint endpoint)
{
    auto* netSock = static_cast<NetworkSocket*>(pipe.device_specific());
    netSock->m_tid = Task::Manager::instance().get_current_tid();
    reinterpret_cast<TransmissionControlProtocol::Socket*>(netSock->m_protocolSock)->connect(endpoint);
    /// FIXME: We should block here until we connect.
    return true;
}

void NetworkSocket::receive(uint8_t* data, uint32_t size)
{
    LockAcquirer l(m_lock);
    
    auto socketWasEmpty = m_queue.is_queue_empty();
    auto writeCount = m_queue.write(size, data);

    if (writeCount > 0) {
        m_readWaitQueue.wake_queue();
        if (socketWasEmpty)
            m_eventListenerList.notify(Pipes::EventTypes::READABLE);
    }
}

std::size_t NetworkSocket::read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific)
{
    auto* socket = static_cast<NetworkSocket*>(deviceSpecific);
    LockAcquirer l(socket->m_lock);

    Task::WaitQueue::Item wqitem = socket->m_readWaitQueue.add_to_queue();
    while (socket->m_queue.is_queue_empty()) {
        if (socket->m_shouldNotBlock)
            return 0;   
        socket->m_lock.release();
        Task::Manager::instance().block();
        socket->m_lock.acquire();
    }

    socket->m_readWaitQueue.remove_from_queue(wqitem);

    return socket->m_queue.read(count, buf);
}

std::size_t NetworkSocket::write(std::size_t offset, std::size_t count, const void* buf, void*& deviceSpecific)
{
    auto* socket = static_cast<NetworkSocket*>(deviceSpecific);
    LockAcquirer l(socket->m_lock);
    
    reinterpret_cast<TransmissionControlProtocol::Socket*>(socket->m_protocolSock)->send(reinterpret_cast<const uint8_t*>(buf), count);

    return count;
}

Pipes::EventListenerList::Receipt NetworkSocket::notify(void* listener, Pipes::raise_events_callback raise_event, Pipes::EventTypeMask& current, void*& deviceSpecific)
{
    auto* socket = static_cast<NetworkSocket*>(deviceSpecific);

    Pipes::EventTypeMask mask = 0;

    LockAcquirer acquire(socket->m_lock);
    if (!socket->m_queue.is_queue_empty())
        mask |= Pipes::EventTypes::READABLE;
    
    if (socket->m_connections > 0)
        mask |= NetworkSocket::EventTypes::SOCK_REQUEST_CONN;

    mask |= Pipes::EventTypes::WRITEABLE;
    
    current = mask;
    
    return socket->m_eventListenerList.add(listener, raise_event);
}

void NetworkSocket::denotify(Pipes::EventListenerList::Receipt receipt, void*& deviceSpecific)
{
    auto* socket = static_cast<NetworkSocket*>(deviceSpecific);
    LockAcquirer acquire(socket->m_lock);

    socket->m_eventListenerList.remove(receipt);
}

}
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

#include "Networking/TCP/Common.h"
#include "Networking/TCP/Receive.h"
#include "Networking/TCP/Socket.h"
#include "Networking/IP/IPv4/Common.h"
#include "Networking/IP/IPv4/Send.h"
#include "Networking/NetworkSocket.h"
#include "Memory/KernelHeap.h"

namespace Networking::TransmissionControlProtocol
{

void Socket::initialise()
{
    m_globalSocketMap = new Common::Hashmap<Endpoint, Common::List<Socket*>>();
}

void Socket::receive_ip(InternetProtocolAddress destIp, InternetProtocolAddress sourceIp, uint8_t* payload, uint32_t size)
{
    auto header = reinterpret_cast<Header*>(payload);
    uint16_t localPort = header->get_dest_port();
    uint16_t remotePort = header->get_source_port();

    Socket* socket = nullptr;

    {
        LockAcquirer l(m_globalSocketLock);
        auto it = m_globalSocketMap->find({localPort, destIp});
        if (it == m_globalSocketMap->end())
            return;
        
        for (auto sockPointer : it->second) {
            if (sockPointer->m_localPort == localPort
            && sockPointer->m_localIp == destIp
            && sockPointer->m_state == State::LISTEN) {
                sockPointer->m_remotePort = remotePort;
                sockPointer->m_remoteIp = sourceIp;
            }

            if (sockPointer->m_localPort == localPort
            && sockPointer->m_localIp == destIp
            && sockPointer->m_remotePort == remotePort
            && sockPointer->m_remoteIp == sourceIp) {
                socket = sockPointer;
                break;
            }
        }
    }

    if (socket)
        socket->receive(sourceIp, payload, size);
}

void Socket::accept_on_listen(const Header& header)
{
    // We have not yet sent anything to be acknowledged so drop the packet if it sets the ACK flag.
    if (header.get_flags() & Flags::ACK)
        return;
    
    // We need SYN flag to establish a connection.
    if (!(header.get_flags() & Flags::SYN))
        return;
    
    rcv_nxt = irs + 1;
    irs = header.get_sequence_number();

    snd_una = header.get_ack_number();
    snd_wnd = header.get_window_size();
    snd_wl1 = header.get_sequence_number();
    snd_wl2 = header.get_ack_number();

    m_state = State::SYN_RECEIVED;
    send(snd_nxt, 0, 0, Flags::SYN | Flags::ACK);
}

void Socket::receive_on_syn_sent(const Header& header)
{
    if (header.get_flags() & Flags::ACK) {
        if (header.get_ack_number() <= iss || header.get_ack_number() > snd_nxt) {
            // reset?
            return;
        }
    }

    if (!(header.get_flags() & Flags::SYN))
        return;

    irs = header.get_sequence_number();
    rcv_nxt = header.get_sequence_number() + 1;
    if (header.get_flags() & Flags::ACK) {
        snd_una = header.get_ack_number();
        snd_wnd = header.get_window_size();
        snd_wl1 = header.get_sequence_number();
        snd_wl2 = header.get_ack_number();

        m_state = State::ESTABLISHED;
        send(snd_nxt, 0, 0, Flags::ACK);
    } else {
        m_state = State::SYN_RECEIVED;
        // Our SYN that we sent has not been acknowledged, send it again.
        snd_nxt--;
        send(snd_nxt, 0, 0, Flags::SYN | Flags::ACK);
    }
}

void Socket::refresh_retransmission_queue()
{
    for (auto it = m_retransmissionQueue.begin(); it != m_retransmissionQueue.end(); ++it)
    {
        if (it->sequenceNumber < snd_una) {
            it = m_retransmissionQueue.erase(it);
            continue;
        }

        /*if (pit_ticks >= expiry)
        {

            it = m_retransmissionQueue.remove(it);
            // remove and resend
            send(it->sequenceNumber, (uint8_t*)it->data, it->length, Flags::ACK);
        }*/
    }
}

void Socket::receive_ack(const Header& header)
{
    switch (m_state) {

    case State::SYN_RECEIVED:
        // Check whether the acknowledgement number is in the send window.
        if (snd_una <= header.get_ack_number() && header.get_ack_number() <= snd_nxt) {
            snd_wnd = header.get_window_size();
            // SND_WL1 is the sequence number of the packet that last updated the window.
            snd_wl1 = header.get_sequence_number();
            // SND_WL2 is the ack number of the packet that last updated the window.
            snd_wl2 = header.get_ack_number();
            m_state = State::ESTABLISHED;
        }
        else {
            send(header.get_ack_number(), 0, 0, Flags::RST | Flags::ACK);
        }
        break;
    
    case State::ESTABLISHED:
        
        if (snd_una <= header.get_ack_number() && header.get_ack_number() <= snd_nxt) {
            // We have now received an ack for the first bytes in the send window.
            // Therefore, move the send window up.
            snd_una = header.get_ack_number();

            // We need to update the window
            if (snd_wl1 < header.get_sequence_number() || (snd_wl1 == header.get_sequence_number() && snd_wl2 < header.get_ack_number())) {
                snd_wnd = header.get_window_size();
                snd_wl1 = header.get_sequence_number();
                snd_wl2 = header.get_ack_number();
            }

            // We store the unacknowledged packets in the retransmission queue in case we need to send them again.
            // Update the queue to remove the now acknowledged packets.
            refresh_retransmission_queue();
        }
    
    default:
        break;
    }
}

void Socket::receive_rst(const Header&)
{
    /// TODO: Implement.
}

void Socket::receive_fin(const Header& header)
{
    // We just acknowledge the FIN.
    rcv_nxt = header.get_sequence_number() + 1;
    send(snd_nxt, 0, 0, Flags::ACK);

    switch (m_state) {
    case State::SYN_RECEIVED:
    case State::ESTABLISHED:
        m_state = State::CLOSE_WAIT;
        break;
    case State::FIN_WAIT1:
        if (header.get_ack_number() > snd_nxt) {
            m_state = State::TIME_WAIT;
        } else {
            m_state = State::CLOSING;
        }
        break;
    case State::FIN_WAIT2:
    case State::TIME_WAIT:
        m_state = State::TIME_WAIT;
        break;
    default:
        break;
    }
}

void Socket::receive_on_listen(InternetProtocolAddress sourceIp, const Header& header)
{
    Endpoint remote = {header.get_source_port(), sourceIp};
    m_connectionQueue.push_back({remote, &header});
    
}

void Socket::add_to_received(const Packet& packet)
{
    auto it = m_outOfOrderList.begin();
    for (; it != m_outOfOrderList.end(); ++it) {
        if (packet.sequenceNumber < it->sequenceNumber)
            break;
    }
    m_outOfOrderList.insert(it, packet);
}

void Socket::process_received()
{
    for (auto it = m_outOfOrderList.begin(); it != m_outOfOrderList.end();) {
        // If we now have packets that were previously missing, we can send them to the application.
        if (rcv_nxt != it->sequenceNumber)
            break;
        
        rcv_nxt += it->length;
        
        reinterpret_cast<NetworkSocket*>(m_netSock)->receive(reinterpret_cast<uint8_t*>(it->data), it->length);
        
        it = m_outOfOrderList.erase(it);
    }
}

void Socket::receive_data(uint8_t* data, uint16_t size)
{
    auto header = reinterpret_cast<Header*>(data);
    auto headerSize = header->get_data_offset() * 4;

    if (headerSize >= size)
        return;

    switch(m_state) {
    case State::ESTABLISHED:
        Packet packet;
        packet.data = data+headerSize;
        packet.length = size-headerSize;
        packet.sequenceNumber = header->get_sequence_number();
        // Add to the received queue.
        add_to_received(packet);
        // Check if we can send the received packets to the application layer.
        process_received();
        // Acknowledge the packet.
        send(snd_nxt, 0, 0, Flags::ACK);
        break;
    default:
        break;
    }
}

void Socket::receive(InternetProtocolAddress sourceIp, uint8_t* data, uint16_t size)
{
    auto sizeInclPseudoHeader = sizeof(PseudoHeader) + size;
    auto buf = reinterpret_cast<uint8_t*>(kmalloc(sizeInclPseudoHeader, 0));

    memcpy(buf + sizeof(PseudoHeader), data, size);
    auto pseudoHeader = reinterpret_cast<PseudoHeader*>(buf);
    auto header = reinterpret_cast<Header*>(buf + sizeof(PseudoHeader));
    pseudoHeader->set_dest_ip(InternetProtocolV4::ipAddress);
    pseudoHeader->set_source_ip(sourceIp);
    pseudoHeader->set_length(size);
    pseudoHeader->set_protocol(InternetProtocolV4::Protocol::TCP);
    
    //if (InternetProtocolV4::checksum(reinterpret_cast<uint16_t*>(buf), sizeInclPseudoHeader))
    //    return;
    header = reinterpret_cast<Header*>(data);

    switch (m_state) {
    case State::LISTEN:
        return receive_on_listen(sourceIp, *header);
    case State::SYN_SENT:
        return receive_on_syn_sent(*header);
    default:
        break;
    }

    if (header->get_flags() & Flags::RST)
        return receive_rst(*header);
    
    // We have already handled the case that we are listening or trying to connect, so if we have
    // a SYN flag (used to intialise connections), we need to reset the connection.

    if (header->get_flags() & Flags::SYN)
        return send(0, 0, 0, Flags::RST | Flags::ACK);
    
    if (!(header->get_flags() & Flags::ACK))
        return;
    
    receive_ack(*header);
    receive_data(data, size);

    if (header->get_flags() & Flags::FIN)
        return receive_fin(*header);
}

void Socket::send(uint32_t seq, const uint8_t* data, uint16_t size, uint8_t flags)
{
    auto sizeInclPseudoHeader = sizeof(PseudoHeader) + sizeof(Header) + size;
    auto buf = reinterpret_cast<uint8_t*>(kmalloc(sizeInclPseudoHeader, 0));
    auto pseudoHeader = reinterpret_cast<PseudoHeader*>(buf);
    auto header = reinterpret_cast<Header*>((buf + sizeof(PseudoHeader))); 

    if (flags & Flags::ACK)
        header->set_ack_number(rcv_nxt);
    else
        header->set_ack_number(0);
    
    header->set_sequence_number(seq);
    header->set_flags(flags);
    header->set_source_port(m_localPort);
    header->set_dest_port(m_remotePort);
    header->set_window_size(WINDOW_SIZE);
    header->set_checksum(0);
    header->set_urgent_pointer(0);

    /// FIXME: Provide proper options support rather a magic number.
    if (flags & Flags::SYN)
        header->set_options(0x020405B4);
    else
        header->set_options(0);
    
    header->set_data_offset(sizeof(Header) / 4);
    pseudoHeader->set_dest_ip(m_remoteIp);
    pseudoHeader->set_source_ip(Networking::InternetProtocolV4::ipAddress);
    pseudoHeader->set_length(sizeof(Header) + size);
    pseudoHeader->set_protocol(InternetProtocolV4::Protocol::TCP);
    
    memcpy(buf + sizeof(PseudoHeader) + sizeof(Header), data, size);

    Networking::Utilities::NetworkEndian<uint16_t> chksum;
    chksum.set_raw_value(InternetProtocolV4::checksum(reinterpret_cast<uint16_t*>(buf), sizeInclPseudoHeader));
    header->set_checksum(chksum.get_value());
    InternetProtocolV4::send(m_remoteIp, InternetProtocolV4::Protocol::TCP, reinterpret_cast<uint8_t*>(header), size + sizeof(Header));

    snd_nxt += size;
    if (flags & (Flags::FIN | Flags::SYN))
        snd_nxt++;
    
    //kfree(buf);
}

Socket* Socket::create_socket(Endpoint endpoint, void* netSock)
{
    LockAcquirer l(m_globalSocketLock);
    
    auto it = m_globalSocketMap->find(endpoint);

    if (it == m_globalSocketMap->end()) 
        it = m_globalSocketMap->insert({endpoint, Common::List<Socket*>()}).first;
    
    auto sock = new Socket(netSock);
    it->second.push_back(sock);

    sock->m_localPort = endpoint.first;
    sock->m_localIp = endpoint.second;
    return sock;
}

bool Socket::accept_conn_request(Socket*& newSock, void* netSock)
{
    if (m_connectionQueue.size() == 0)
        return false;
    
    auto connection = *m_connectionQueue.begin();
    auto endpoint = connection.first;
    auto header = connection.second;

    newSock = create_socket({m_localPort, m_localIp}, netSock);
    newSock->m_remotePort = endpoint.first;
    newSock->m_remoteIp = endpoint.second;
    newSock->accept_on_listen(*header);
    return true;
}

void Socket::listen()
{
    m_state = State::LISTEN;
}

void Socket::connect(Endpoint to)
{
    m_remotePort = to.first;
    m_remoteIp = to.second;

    m_state = State::SYN_SENT;
    send(snd_nxt, 0, 0, Flags::SYN);
}

void Socket::send(const uint8_t* data, std::size_t size)
{
    send(snd_nxt, data, size, Flags::ACK);
}

}
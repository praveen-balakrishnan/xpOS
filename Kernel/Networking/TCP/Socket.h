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

#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "Networking/TCP/Common.h"

namespace Networking::TransmissionControlProtocol
{

struct Packet
{
    uint32_t sequenceNumber;
    uint32_t length;
    void* data;
};

class Socket
{
public:
    static Socket* create_socket(Endpoint endpoint, void* netSock);
    bool accept_conn_request(Socket*& newSock, void* netSock);
    void listen();
    void connect(Endpoint to);
    void send(const uint8_t* data, std::size_t size);
    static void initialise();
    static void receive_ip(InternetProtocolAddress destIp, InternetProtocolAddress sourceIp, uint8_t* payload, uint32_t size);

private:
    Socket(void* netSock) : m_netSock(netSock)
    {}

    static inline Common::Hashmap<Endpoint, Common::List<Socket*>>* m_globalSocketMap;
    static inline Mutex m_globalSocketLock;

    void receive(InternetProtocolAddress sourceIp, uint8_t* data, uint16_t size);
    void accept_on_listen(const Header& header);
    void receive_on_listen(InternetProtocolAddress sourceIp, const Header& header);
    void receive_on_syn_sent(const Header& header);
    void receive_ack(const Header& header);
    void receive_rst(const Header& header);
    void receive_fin(const Header& header);
    void receive_data(uint8_t* data, uint16_t size);
    void refresh_retransmission_queue();
    void send(uint32_t seq, const uint8_t* data, uint16_t size, uint8_t flags);

    void add_to_received(const Packet& packet);
    void process_received();

    enum class State
    {
        LISTEN,
        SYN_SENT,
        SYN_RECEIVED,
        ESTABLISHED,
        FIN_WAIT1,
        FIN_WAIT2,
        CLOSE_WAIT,
        CLOSING,
        LAST_ACK,
        TIME_WAIT,
        CLOSED
    };

    State m_state;

    // These are variables declared by the TCP specification.
    static constexpr uint32_t WINDOW_SIZE = 8192;

    uint32_t snd_una = 0xbeef0101;
    uint32_t snd_nxt = 0xbeef0101;
    uint32_t snd_wnd = WINDOW_SIZE;
    uint32_t snd_up = 0;
    uint32_t snd_wl1 = 0;
    uint32_t snd_wl2 = 0;
    uint32_t iss = 0xbeef0101;
    uint32_t rcv_nxt = 0;
    uint32_t rcv_wnd = 8192;
    uint32_t rcv_up = 0;
    uint32_t irs = 0;

    PortNumber m_localPort;
    PortNumber m_remotePort;
    InternetProtocolAddress m_localIp;
    InternetProtocolAddress m_remoteIp;

    bool m_shouldNotBlock = true;
    Task::TaskID m_tid = 0;

    void* m_netSock = nullptr;

    Common::List<Packet> m_retransmissionQueue;
    Common::List<Packet> m_outOfOrderList;
    Common::List<std::pair<Endpoint, const Header*>> m_connectionQueue;

};

}

#endif
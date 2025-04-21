#ifndef IPC_SERVER_H
#define IPC_SERVER_H

#include <cstdint>
#include <list>
#include <optional>
#include "Libraries/OSLib/Pipe.h"
#include "Libraries/OSLib/Socket.h"
#include "Libraries/OSLib/EventListener.h"
#include "Libraries/IPCLib/Connection.h"

namespace xpOS::IPC
{

template<typename ReceiveEndpoint>
class Server
{
public:
    Server(const char* listenOn)
        : m_listeningSocket(xpOS::OSLib::popen(
            "socket::local",
            nullptr,
            static_cast<xpOS::API::EventType>(xpOS::OSLib::LocalSocket::Flags::NON_BLOCKING)
        ))
    {
        xpOS::OSLib::LocalSocket::bind(m_listeningSocket, listenOn);
        xpOS::OSLib::LocalSocket::listen(m_listeningSocket);
    }

    Server(Server&) = delete;
    Server& operator=(Server&) = delete;

    template <class Handler>
    void start()
    {
        xpOS::OSLib::EventListener listener;
        static constexpr int MAX_EVENTS = 10;
        xpOS::API::Event events[MAX_EVENTS];
        listener.add(m_listeningSocket, static_cast<xpOS::API::EventTypeMask>(xpOS::OSLib::LocalSocket::EventTypes::SOCK_REQUEST_CONN));
        int noRaised = 0;
        while (true) {
            noRaised = listener.listen(events, MAX_EVENTS);
            for (int i = 0; i < noRaised; i++) {
                if (events[i].pd == m_listeningSocket) {
                    accept_new_connections(listener);
                } else {
                    for (auto& client : m_clientSockets) {
                        if (client.pd == events[i].pd) {
                            (*client.connection).template receive_message<Handler>();
                        }
                    }
                }
            }

        }
    }

private:
    struct PipeConnectionPair
    {
        uint64_t pd;
        std::optional<Connection<ReceiveEndpoint>> connection;
    };

    void accept_new_connections(xpOS::OSLib::EventListener& listener)
    {
        uint64_t newClient = 0;
        do {
            newClient = xpOS::OSLib::LocalSocket::accept(m_listeningSocket);
            if (newClient > 0) {
                m_clientSockets.push_back({newClient, std::nullopt});
                auto it = --m_clientSockets.end();
                listener.add(it->pd, xpOS::API::EventTypes::READABLE);
                it->connection = std::optional(Connection<ReceiveEndpoint>(it->pd));
            }
        } while (newClient > 0);
    }

    uint64_t m_listeningSocket;
    std::list<PipeConnectionPair> m_clientSockets;
};

}

#endif
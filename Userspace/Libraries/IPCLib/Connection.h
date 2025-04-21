#ifndef IPCLIB_CONNECTION_H
#define IPCLIB_CONNECTION_H

#include "Libraries/SerialisationLib/DeserialisedData.h"
#include "Libraries/SerialisationLib/SerialisedData.h"
#include "Libraries/OSLib/EventListener.h"
#include "Libraries/OSLib/Pipe.h"
#include "Libraries/OSLib/Socket.h"
#include <list>
#include <vector>

namespace xpOS::IPC
{

using namespace xpOS;

template <typename ReceiveEndpointDefinition>
class Connection;

template<class Handler, class SpecificMessage, class ReceiveEndpointDefinition>
void receive_ipc_message(Serialisation::DeserialisedData& m, Connection<ReceiveEndpointDefinition>& c)
{
    SpecificMessage s;
    s << m;
    Handler::handle_ipc_message(std::move(s), c);
}

template <typename... MessageTypes>
struct Endpoint
{};

// conn should be able to read a message when we know we have one
// i.e. read_as_many_as_possible_without_blocking


// should be able to process a message and switch on it..


// MUST BE USED WITH NONBLOCKING SOCKETS
template <typename ReceiveEndpointDefinition>
class Connection
{
public:
    Connection(uint64_t nonBlockingSocket)
        : m_socket(nonBlockingSocket)
    {
    }

    Connection(Connection&) = delete;
    Connection& operator=(Connection&) = delete;

    Connection(Connection&& conn)
        : m_socket(0)
    {
        swap(conn, *this);
    }

    Connection& operator=(Connection&& conn)
    {
        swap(conn, *this);
        return *this;
    }

    friend void swap(Connection& a, Connection& b)
    {
        using std::swap;
        swap(a.m_socket, b.m_socket);
        swap(a.m_unprocessedMessages, b.m_unprocessedMessages);
    }

    /*Connection(Connection&& conn)
        : m_socket(nullptr)
    {
        swap(conn, *this);
    }

    Connection& operator=(Connection&& conn)
    {
        swap(conn, *this);
        return *this;
    }

    friend void swap(Connection& a, Connection& b)
    {
        using std::swap;
        swap(a.m_socket, b.m_socket);
        swap(a.m_unprocessedMessages, b.m_unprocessedMessages);
    }*/

    template <typename MessageType>
    void send_message(MessageType m)
    {
        Serialisation::SerialisedData serialised;
        m >> serialised;
        MessageHeader header = {
            .messageId = MessageType::MessageId,
            .length = serialised.length()
        };
        
        force_write(sizeof(MessageHeader), &header);
        force_write(serialised.length(), serialised.bytes());
    }

    void read_maximal_messages()
    {
        while (try_to_read_message()) {}
    }

    bool try_to_read_message()
    {
        MessageHeader header;
        
        /// FIXME: We might unfortunately read less than the sizeof(MessageHeader).
        if (xpOS::OSLib::pread(m_socket, &header, sizeof(MessageHeader)) == 0)
            return false;
        
        std::vector<uint8_t> dataBuffer(header.length);
        force_read(header.length, dataBuffer.data());
        
        m_unprocessedMessages.push_back({header.messageId, Serialisation::DeserialisedData(dataBuffer.data(), header.length)});
        return true;
    }

    /*void wait_for_new_message()
    {
        bool wholeMessageRead = false;

        int totalBytes = 0;

        MessageHeader header;
        force_read(sizeof(MessageHeader), &header);
        
        Common::Vector<uint8_t> dataBuffer(header.length);
        force_read(header.length, dataBuffer.data());
        
        m_unprocessedMessages.push_back({header.messageId, Serialisation::DeserialisedData(dataBuffer.data(), header.length)});
    }*/

    template <class Handler>
    void receive_message()
    {
        read_maximal_messages();
        for (auto it = m_unprocessedMessages.begin(); it != m_unprocessedMessages.end();) {
            handle_message<Handler>(it->messageId, it->message);
            it = m_unprocessedMessages.erase(it);
        }
    }

    void force_read(std::size_t size, void* buf)
    {
        std::size_t totalBytesRead = 0;
        while (totalBytesRead < size) {
            totalBytesRead += xpOS::OSLib::pread(m_socket, static_cast<uint8_t*>(buf) + totalBytesRead, size - totalBytesRead);
        }
    }

    void force_write(std::size_t size, const void* buf)
    {
        std::size_t totalBytesWritten = 0;
        while (totalBytesWritten < size) {
            totalBytesWritten += xpOS::OSLib::pwrite(m_socket, static_cast<const uint8_t*>(buf) + totalBytesWritten, size - totalBytesWritten);
        }
    }

    /*template <class Handler>
    void listen_for_messages()
    {
        while (true) {
            for (auto it = m_unprocessedMessages.begin(); it != m_unprocessedMessages.end();) {
                handle_message<Handler>(it->messageId, it->message);
                it = m_unprocessedMessages.erase(it);
            }
            wait_for_new_message();
        }
    }*/

    template <typename MessageType>
    MessageType get_message_of_type()
    {
        while (true) {
            auto it = m_unprocessedMessages.begin();
            for (; it != m_unprocessedMessages.end(); it++) {
                if (it->messageId == MessageType::MessageId)
                    break;
            }

            if (it != m_unprocessedMessages.end()) {
                Serialisation::DeserialisedData data = std::move(it->message);
                m_unprocessedMessages.erase(it);
                MessageType message;
                message << data;
                return message;
            }

            read_maximal_messages();
        }
        
    }

    template <typename MessageType, typename ReplyMessageType>
    ReplyMessageType send_and_wait_for_reply(MessageType& m)
    {
        send_message(m);
        return get_message_of_type<ReplyMessageType>();
    }

    template <class Handler>
    bool handle_message(int messageType, Serialisation::DeserialisedData& message)
    {
        return handle_message<Handler>(ReceiveEndpointDefinition(), messageType, message);
    }

    template <class Handler, typename... MessageType>
    bool handle_message(Endpoint<MessageType...>, int messageType, Serialisation::DeserialisedData& message)
    {
        return (
            (messageType == MessageType::MessageId && (receive_ipc_message<Handler, MessageType, ReceiveEndpointDefinition>(message, *this), true))
            || ... 
        );
    }

    template <class Handler>
    void handle_messages_indefinitely()
    {
        OSLib::EventListener listener;
        API::Event event;
        listener.add(m_socket, API::EventTypes::READABLE);
        while (true) {
            if (listener.listen(&event, 1))
                receive_message<Handler>();
            else
                break;
        }
    }

private:
    struct QueuedMessage
    {
        int messageId;
        Serialisation::DeserialisedData message;
    };

    struct MessageHeader
    {
        int messageId;
        std::size_t length;
    }__attribute__((packed));

    uint64_t m_socket;
    std::list<QueuedMessage> m_unprocessedMessages;
};

}

#endif
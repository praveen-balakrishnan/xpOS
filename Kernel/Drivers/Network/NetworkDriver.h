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

#ifndef NETWORKDRIVER_H
#define NETWORKDRIVER_H

#include <cstdint>
#include <API/Network.h>
#include "Common/List.h"

namespace Drivers
{
    class EthernetNetworkDriverEventHandler;
    class EthernetNetworkDriver
    {
    friend class EthernetNetworkDriverEventHandler;
    public:
        virtual Networking::MediaAccessControlAddress get_mac_address() = 0;
        virtual Networking::InternetProtocolAddress get_ip_address() = 0;
        virtual void set_ip_address(Networking::InternetProtocolAddress ipAddr) = 0;
        virtual void send_data(uint8_t* buf, uint64_t size) = 0;
    protected:
        Common::List<EthernetNetworkDriverEventHandler*> m_handlers;
        void add_handler(EthernetNetworkDriverEventHandler* handler);
        void remove_handler(EthernetNetworkDriverEventHandler* handler);
    };

    class EthernetNetworkDriverEventHandler
    {
    public:
        virtual void on_receive_data(uint8_t* buf, uint64_t size) = 0;
        EthernetNetworkDriverEventHandler(EthernetNetworkDriver* driver);
        ~EthernetNetworkDriverEventHandler();
    protected:
        EthernetNetworkDriver* m_driver;
    };
}

#endif
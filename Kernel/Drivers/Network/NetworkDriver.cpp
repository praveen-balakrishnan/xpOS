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

#include "Drivers/Network/NetworkDriver.h"

namespace Drivers
{
    void EthernetNetworkDriver::add_handler(EthernetNetworkDriverEventHandler* handler)
    {
        m_handlers.push_back(handler);
    }

    void EthernetNetworkDriver::remove_handler(EthernetNetworkDriverEventHandler* handler)
    {
        auto it = m_handlers.begin();
        for (; it != m_handlers.end(); ++it) {
            if (*it == handler) { break; }
        }
        if (*it == handler) { m_handlers.erase(it); };
    }

    EthernetNetworkDriverEventHandler::EthernetNetworkDriverEventHandler(EthernetNetworkDriver* driver) : m_driver(driver)
    {
        m_driver->add_handler(this);
    }

    EthernetNetworkDriverEventHandler::~EthernetNetworkDriverEventHandler() { m_driver->remove_handler(this); }

}
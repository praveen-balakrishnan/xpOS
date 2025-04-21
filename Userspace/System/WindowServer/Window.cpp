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

#include "Window.h"
#include <API/SharedMemory.h>
#include "ClientEndpoint.h"

using namespace xpOS;

Context::Context(const std::string& bufferId, int width, int height, IPC::Connection<WSEndpoint>* conn)
    : m_conn(conn)
{
    m_buffer.width = width;
    m_buffer.height = height;
    API::SharedMemory::LinkRequest req = {
        .str = bufferId.c_str(),
        .mappedTo = nullptr,
        .length = width * height * sizeof(uint32_t)
    };
    // We open a shared framebuffer that the callee can open and populate.
    m_sharedMemoryFd = OSLib::popen("shared_mem", &req, API::SharedMemory::Flags::CREATE);
    m_buffer.buffer = static_cast<uint32_t*>(req.mappedTo);
}

Context::~Context()
{
    OSLib::pclose(m_sharedMemoryFd);
}

void Context::handle_keyboard_event(API::HID::KeyboardEvent apiEvent)
{
    KeyEvent kEvent;
    kEvent.keyUp = apiEvent.movement == xpOS::API::HID::KeyboardEvent::KEY_UP;
    kEvent.key = apiEvent.key;
    m_conn->send_message(kEvent);
}

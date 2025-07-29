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

#ifndef XPOS_GUILIB_WINDOW_H
#define XPOS_GUILIB_WINDOW_H

#include <optional>
#include "Context.h"
#include "Libraries/IPCLib/Connection.h"
#include "Libraries/OSLib/Socket.h"
#include "Libraries/OSLib/Pipe.h"
#include "System/WindowServer/ClientEndpoint.h"
#include "System/WindowServer/ServerEndpoint.h"

namespace xpOS::GUILib
{

using namespace xpOS::System::WindowServer;

class Window
{
private:
    Window(uint32_t* framebuffer, Size size, IPC::Connection<ClientEndpoint> wsconn)
    : m_framebuffer(framebuffer)
    , m_size(size)
    , m_wsconn(std::move(wsconn))
    {}

    uint32_t* m_framebuffer;
    Size m_size;
    IPC::Connection<ClientEndpoint> m_wsconn;
public:
    static std::optional<Window> create(int width, int height);
    Context get_context()
    {
        Context context {
            .framebuffer = m_framebuffer,
            .size = m_size
        };
        return context;
    }
};

}

#endif

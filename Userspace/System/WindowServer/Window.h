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

#ifndef WINDOWSERVER_WINDOW_H
#define WINDOWSERVER_WINDOW_H

#include <API/HID.h>
#include <Libraries/IPCLib/Connection.h>
#include <Libraries/OSLib/Pipe.h>

#include "Geometry.h"
#include "Graphics.h"
#include "ServerEndpoint.h"

#include <list>
#include <memory>

using namespace xpOS;
using namespace xpOS::System::WindowServer;

class Context
{
public:
    Context(const std::string& bufferId, int width, int height, IPC::Connection<WSEndpoint>* conn);
    Context(const Context&) = delete;
    ~Context();

    void handle_keyboard_event(API::HID::KeyboardEvent event);

    const RenderBuffer& get_buffer() const
    {
        return m_buffer;
    }

private:
    RenderBuffer m_buffer;
    uint64_t m_sharedMemoryFd;
    xpOS::IPC::Connection<System::WindowServer::WSEndpoint>* m_conn;
};

class Window
{
public:
    Window(const std::shared_ptr<Context>& context, WorldCoord worldCoord)
        : m_context(context)
        , m_worldCoord(worldCoord)
    {}

    Context* get_context() const
    {
        return m_context.get();
    }

    Rect get_geometry() const
    {
        return Rect {
            .coord = m_worldCoord,
            .width = m_context->get_buffer().width,
            .height = m_context->get_buffer().height,
        };
    }

    void move_window_to(WorldCoord worldCoord)
    {
        m_worldCoord = worldCoord;
    }

    void shift_window_by(long long deltaX, long long deltaY)
    {
        m_worldCoord.x += deltaX;
        m_worldCoord.y += deltaY;
    }

private:
    std::shared_ptr<Context> m_context;
    WorldCoord m_worldCoord;
    Window* m_parent;
};


#endif
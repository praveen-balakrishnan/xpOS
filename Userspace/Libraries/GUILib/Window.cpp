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

#include "API/Event.h"
#include "API/SharedMemory.h"
#include "Window.h"

namespace xpOS::GUILib
{

std::optional<Window> Window::create(int width, int height)
{
    int serverPd = OSLib::popen("socket::local", 0, static_cast<API::EventType>(OSLib::LocalSocket::Flags::NON_BLOCKING));
    while (!OSLib::LocalSocket::connect(serverPd, "xp::WindowServer"));
    auto wsconn = IPC::Connection<ClientEndpoint>(serverPd);

    CreateWindow cw = {
        .width = width,
        .height = height
    };

    auto callback = wsconn.send_and_wait_for_reply<CreateWindow, CreateWindowCallback>(cw);
    API::SharedMemory::LinkRequest req = {
        .str = callback.sharedMemoryId.c_str(),
        .mappedTo = nullptr,
        .length = width * height * sizeof(uint32_t)
    };

    int sharedMemPd = OSLib::popen("shared_mem", &req);
    auto framebuffer = static_cast<uint32_t*>(req.mappedTo);

    return Window(framebuffer, Size(width, height), std::move(wsconn));
}


}
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

#include <API/Framebuffer.h>
#include <API/HID.h>
#include <API/SharedMemory.h>
#include <Libraries/OSLib/Pipe.h>
#include <Libraries/IPCLib/Connection.h>
#include <Libraries/IPCLib/Server.h>
#include <pthread.h>
#include <unordered_map>

#include "ClientEndpoint.h"
#include "ServerEndpoint.h"
#include "HID.h"
#include "InfiniteDesktop.h"
#include "Screen.h"
#include "Window.h"

namespace
{
    using namespace xpOS::System::WindowServer;
    std::unordered_map<xpOS::IPC::Connection<WSEndpoint>*, Window*> windowMap;
    InfiniteDesktop* desktop;
    Screen* screen = nullptr;
    int currentWindowId = 1;

    static constexpr auto SERVER_ID = "xp::WindowServer";
}

namespace xpOS::System::WindowServer
{

struct ServerHandler
{
    static void handle_ipc_message(
        CreateWindow cwindow,
        xpOS::IPC::Connection<WSEndpoint>& conn
    ) 
    {
        static constexpr std::string WS_WINDOW_SHARE_PREFIX = "wsfb::";
        CreateWindowCallback callback {
            .sharedMemoryId = WS_WINDOW_SHARE_PREFIX + std::to_string(currentWindowId++)
        };
        auto context = std::make_shared<Context>(callback.sharedMemoryId, cwindow.width, cwindow.height, &conn);

        auto* window = desktop->make_window(context, WorldCoord(10 + currentWindowId * 150, 10));
        windowMap.insert({&conn, window});
        conn.send_message(callback);
        desktop->paint_all();
    }

    static void handle_ipc_message(
        FlushWindow fwindow,
        xpOS::IPC::Connection<WSEndpoint>& conn
    ) 
    {
        auto it = windowMap.find(&conn);
        if (it == windowMap.end())
            return;
        desktop->invalidate_window(it->second);
    }
};

}

int main()
{
    using namespace xpOS::System::WindowServer;
    screen = &Screen::main();
    //desktop.bind_to_screen(screen);
    desktop = new InfiniteDesktop(screen->get_buffer());
    desktop->paint_all();
    screen->flush();

    Environment e {
        .desktop = desktop,
        .screen = screen
    };

    /// FIXME: Our C++ toolchain does not support threads yet, so once it does
    /// we should use C++ threads.
    pthread_t hidThread;
    pthread_create(&hidThread, NULL, hid_thread, &e);

    IPC::Server<System::WindowServer::WSEndpoint> server(SERVER_ID);
    server.start<ServerHandler>();
    return 0;
}

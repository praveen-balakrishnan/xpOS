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

#include <Libraries/OSLib/Pipe.h>
#include <Libraries/OSLib/EventListener.h>

#include "Geometry.h"
#include "HID.h"
#include "InfiniteDesktop.h"

namespace xpOS::System::WindowServer
{

using namespace xpOS;

namespace
{
    int mouseX = 100;
    int mouseY = 100;
}

void read_and_handle_keyboard_events(uint64_t keyboardPd, InfiniteDesktop* desktop, Screen* screen)
{
    static constexpr int MAX_EVENTS = 10;
    API::HID::KeyboardEvent keyEvents[MAX_EVENTS];
    OSLib::pread(keyboardPd, &keyEvents, MAX_EVENTS * sizeof(API::HID::KeyboardEvent));

    desktop->process_keyboard_event(keyEvents[0]);
}

void read_and_handle_mouse_events(uint64_t mousePd, InfiniteDesktop* desktop, Screen* screen)
{
    static constexpr int MAX_EVENTS = 10;
    API::HID::MouseEvent mouseEvents[MAX_EVENTS];
    OSLib::pread(mousePd, &mouseEvents, MAX_EVENTS * sizeof(API::HID::MouseEvent));
    
    mouseX += mouseEvents[0].dx;
    mouseY += mouseEvents[0].dy;
    ViewportCoord vc(mouseX, mouseY);
    desktop->process_mouse_event(vc, mouseEvents[0].buttons);
    // We flush the screen as the cursor has been moved.
    screen->flush();
}

void* hid_thread(void* env)
{
    auto* environment = reinterpret_cast<Environment*>(env);
    auto* desktop = environment->desktop;
    auto* screen = environment->screen;

    auto mousePd = OSLib::popen("hid::mouse");
    auto keyboardPd = OSLib::popen("hid::keyboard");
    OSLib::EventListener listener;
    listener.add(mousePd, API::EventTypes::READABLE);
    listener.add(keyboardPd, API::EventTypes::READABLE);

    static constexpr int MAX_EVENTS = 5;
    API::Event events[MAX_EVENTS];

    while (true) {
        int raised = listener.listen(events, MAX_EVENTS);
        for (int i = 0; i < raised; i++) {
            if (events[i].pd == mousePd)
                read_and_handle_mouse_events(mousePd, desktop, screen);
            else if (events[i].pd == keyboardPd)
                read_and_handle_keyboard_events(keyboardPd, desktop, screen);
        }
    }
}

ViewportCoord get_mouse_coord()
{
    return ViewportCoord{
        .x = mouseX,
        .y = mouseY
    };
}

}
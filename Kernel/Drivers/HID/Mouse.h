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

#ifndef HID_MOUSE_H
#define HID_MOUSE_H

#include <cstdint>

#include "Pipes/Pipe.h"

namespace Devices::HID::Mouse
{
    static constexpr uint8_t IRQ_LINE = 0x2C;
    static constexpr uint8_t DATA_PORT = 0x60;
    static constexpr uint8_t COMMAND_PORT = 0x64;

    static constexpr uint8_t MOUSE_BUTTONS_MASK = 0x7;

    bool open(void* with, int flags, void*& deviceSpecific);
    void close(void*& deviceSpecific);
    std::size_t read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific);
    Pipes::EventListenerList::Receipt notify(void* listener, Pipes::raise_events_callback raise_event, Pipes::EventTypeMask& current, void*& deviceSpecific);
    void denotify(Pipes::EventListenerList::Receipt receipt, void*& deviceSpecific);

    void irq_handler();
    void initialise();
}

#endif
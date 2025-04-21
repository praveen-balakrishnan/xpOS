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

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "API/HID.h"
#include "Pipes/Pipe.h"

/**
 * Currently we only support Scancode Set 1 PS/2 keyboards.
 */
namespace Devices::HID::Keyboard
{
    static constexpr uint8_t IRQ_LINE = 0x21;
    static constexpr uint8_t SCANCODE_PORT = 0x60;

    bool open(void* with, int flags, void*& deviceSpecific);
    void close(void*& deviceSpecific);
    std::size_t read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific);
    Pipes::EventListenerList::Receipt notify(void* listener, Pipes::raise_events_callback raise_event, Pipes::EventTypeMask& current, void*& deviceSpecific);
    void denotify(Pipes::EventListenerList::Receipt receipt, void*& deviceSpecific);

    void irq_handler();

    void initialise();

    using CK = xpOS::API::HID::ControlKeys;

    static constexpr unsigned char SCANCODE_S1_MAP[128] =
    {
        0, CK::ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', CK::BACKSPACE,
        CK::TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', CK::RETURN,
        CK::CTRL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',
        0, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, 0,
        0, ' '
    };

    struct SCANCODE_S1_EXTCODES
    {
        static constexpr unsigned char CURSOR_UP = 0x48;
        static constexpr unsigned char CURSOR_LEFT = 0x4B;
        static constexpr unsigned char CURSOR_RIGHT = 0x4D;
        static constexpr unsigned char CURSOR_DOWN = 0x50;
    };

    static constexpr unsigned char SCANCODE_S1_EXTID = 0xE0;
    static constexpr unsigned char SCANCODE_S1_RELEASEMOD = 0x80;

}

#endif
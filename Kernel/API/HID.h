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

#ifndef XPOS_API_HID_H
#define XPOS_API_HID_H

#include <cstdint>

namespace xpOS::API::HID {

struct MouseButtons
{
    static constexpr uint8_t LEFTMB = 0x1;
    static constexpr uint8_t RIGHTMB = 0x2;
    static constexpr uint8_t MIDDLEMB = 0x4;
};

struct MouseEvent
{
    int dx = 0;
    int dy = 0;
    uint8_t buttons = 0;
};

struct KeyboardEvent
{
    enum Movement
    {
        KEY_DOWN, KEY_UP
    } movement;
    unsigned char key;
};

struct ControlKeys
{
    static constexpr unsigned char BACKSPACE    = 0x08;
    static constexpr unsigned char TAB          = 0x09;
    static constexpr unsigned char RETURN       = 0x0D;
    static constexpr unsigned char SHIFT        = 0x10;
    static constexpr unsigned char CTRL         = 0x11;
    static constexpr unsigned char CAPLOCK      = 0x14;
    static constexpr unsigned char ESCAPE       = 0x1B;
    static constexpr unsigned char LARROW       = 0x25;
    static constexpr unsigned char UARROW       = 0x26;
    static constexpr unsigned char RARROW       = 0x27;
    static constexpr unsigned char DARROW       = 0x28;
};

}

#endif
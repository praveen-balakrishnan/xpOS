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

#ifndef WINDOWSERVER_SCREEN_H
#define WINDOWSERVER_SCREEN_H

#include <cstdint>
#include "Graphics.h"

namespace xpOS::System::WindowServer
{

class Screen
{
public:
    static Screen& main()
    {
        static Screen instance;
        return instance;
    }

    void flush();

    const RenderBuffer& get_buffer() const
    {
        return m_buffer;
    }

private:
    Screen();
    RenderBuffer m_buffer;
    //int m_width;
    //int m_height;
    //uint32_t* m_framebuffer;
    uint64_t m_screenpd;
};

}

#endif
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

#include "Screen.h"
#include <API/Framebuffer.h>
#include <API/Syscall.h>
#include "Libraries/OSLib/Pipe.h"
#include "HID.h"
#include "Graphics.h"

using namespace xpOS;

namespace xpOS::System::WindowServer
{

Screen::Screen()
{
    API::Framebuffer::AccessRequest req;
    m_screenpd = OSLib::popen("screen::fb", &req);
    m_buffer.width = req.width;
    m_buffer.height = req.height;
    m_buffer.buffer = static_cast<uint32_t*>(req.mapped);
}

void Screen::flush()
{
    API::Framebuffer::Command cmd;
    cmd.type = API::Framebuffer::Command::FLUSH;
    OSLib::pwrite(m_screenpd, &cmd, sizeof(cmd));
}

}

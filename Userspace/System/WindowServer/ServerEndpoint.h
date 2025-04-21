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

#ifndef WINDOWSERVER_SERVERENDPOINT_H
#define WINDOWSERVER_SERVERENDPOINT_H

#include "Libraries/IPCLib/Connection.h"

namespace xpOS::System::WindowServer
{

struct CreateWindow
{
    static constexpr int MessageId = 1;
    int width;
    int height;

    template<typename Archive>
    void define_archivable(Archive& ar)
    {
        ar(width, height);
    }
};

struct FlushWindow
{
    static constexpr int MessageId = 2;
    //int i;

    template<typename Archive>
    void define_archivable(Archive& ar)
    {
        ar();
    }
};

using WSEndpoint = xpOS::IPC::Endpoint<CreateWindow, FlushWindow>;

}

#endif
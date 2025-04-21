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
#ifndef WINDOWSERVER_WINDOWENDPOINT_H
#define WINDOWSERVER_WINDOWENDPOINT_H

#include "Libraries/IPCLib/Connection.h"

namespace xpOS::System::WindowServer
{
    struct CreateWindowCallback
    {
        static constexpr int MessageId = 3;
        std::string sharedMemoryId;

        template<typename Archive>
        void define_archivable(Archive& ar)
        {
            ar(sharedMemoryId);
        }
    };

    struct KeyEvent
    {
        static constexpr int MessageId = 4;
        bool keyUp;
        unsigned char key;

        template<typename Archive>
        void define_archivable(Archive& ar)
        {
            ar(keyUp, key);
        }
    };

    using ClientEndpoint = xpOS::IPC::Endpoint<KeyEvent>;
}

#endif
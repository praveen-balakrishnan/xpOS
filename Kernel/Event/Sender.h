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

#ifndef SENDER_H
#define SENDER_H

#include <cstdint>

/**
 * WARNING: This code is deprecated and due for deletion. The APIs are no longer
 * exposed or usable.
 * 
 * Asynchronous events are now implemented using the Pipes/EventListeners API. 
 */

namespace Events
{
    struct EventDescriptor;
    class EventSender
    {
    public:
        virtual void register_event_listener(uint64_t ped, uint64_t tid) = 0;
        virtual void block_event_listen(uint64_t ped, uint64_t tid) = 0;
    };
}

#endif
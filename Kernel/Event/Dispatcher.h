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

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "Common/List.h"
#include "Common/Hashmap.h"
#include "Common/String.h"
#include "Event/Sender.h"

/**
 * WARNING: This code is deprecated and due for deletion. The APIs are no longer
 * exposed or usable.
 * 
 * Asynchronous events are now implemented using the Pipes/EventListeners API. 
 */

namespace Task { class Task; }

namespace Events {

    struct EventMessage
    {
        void* message;
    };
    
    struct EventDescriptor;

    struct EventDescriptorItem
    {
        uint64_t processEventDescriptor;
        uint64_t tid;
    };

    struct EventDescriptor
    {
        uint64_t ed;
        void* eventSender = nullptr;
        // The param is a free field that the EventSender can use.
        void* param = nullptr;
        void* process = nullptr;
        Common::List<EventMessage> queue;
        Common::List<Common::List<EventDescriptorItem>*> listenerListElements;
        Common::List<Common::List<EventDescriptorItem>*> waitListElements;
    };

    /*struct FixedLenStringHashContainer
    {
        char str[180];
        friend bool operator==(const FixedLenStringHashContainer& lhs, const FixedLenStringHashContainer& rhs)
        {
            return !strcmp(lhs.str, rhs.str);
        }
    };*/

    class EventDispatcher
    {
    public:
        static EventDispatcher& instance()
        {
            static EventDispatcher instance;
            return instance;
        }
        EventDispatcher(EventDispatcher const&) = delete;
        EventDispatcher& operator=(EventDispatcher const&) = delete;

        // Open an event descriptor to receive event messages for the event.
        static uint64_t register_event_listener(uint64_t tid, char* event, void* param);
        // Close the event descriptor.
        static uint64_t deregister_event_listener(uint64_t tid, uint64_t ped);
        // Read from the event descriptor's queue of messages.
        static uint64_t read_from_event_queue(uint64_t tid, uint64_t ped);
        // Block the process until an event message is received.
        static void block_event_listen(uint64_t tid, uint64_t ed);
        
        // Register as an event sender with an event name.
        void register_event_sender(char* event, EventSender* sender);
        // Send an event message to all listeners.
        void dispatch_event(EventMessage event, Common::List<EventDescriptorItem>* processes);
        void add_process_to_listener_event_list(uint64_t ped, uint64_t tid, Common::List<EventDescriptorItem>* list);
        void add_process_to_wait_event_list(uint64_t ped, uint64_t tid, Common::List<EventDescriptorItem>* list);
        // Wake all processes in the list.
        void wake_processes(Common::List<EventDescriptorItem>* list);
    private:
        EventDispatcher() {}
        Common::Hashmap<Common::HashableString, EventSender*> m_eventSenderHashmap;
        void dispatch_event(EventMessage event, EventDescriptorItem processed);
        void add_process_to_event_list(EventDescriptorItem item, Common::List<EventDescriptorItem>* list);
        void remove_process_from_listener_lists(uint64_t ped, uint64_t tid);
        void remove_process_from_wait_lists(uint64_t ped, uint64_t tid);
    };
}
#endif
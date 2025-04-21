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

#include "Event/Dispatcher.h"
#include "Tasks/TaskManager.h"

/**
 * WARNING: This code is deprecated and due for deletion. The APIs are no longer
 * exposed or usable.
 * 
 * Asynchronous events are now implemented using the Pipes/EventListeners API. 
 */

namespace Events {

    void EventDispatcher::register_event_sender(char* event, EventSender* sender)
    {
        auto str = Common::HashableString(event);
        m_eventSenderHashmap.insert({str, sender});
    }

    void EventDispatcher::add_process_to_event_list(EventDescriptorItem item, Common::List<EventDescriptorItem>* list)
    {
        list->push_back(item);
    } 

    void EventDispatcher::add_process_to_listener_event_list(uint64_t ped, uint64_t tid, Common::List<EventDescriptorItem>* list)
    {
        add_process_to_event_list({ped, tid}, list);
        Task::Manager::instance().get_ed(ped, tid)->second.listenerListElements.push_back(list);

    } 

    void EventDispatcher::add_process_to_wait_event_list(uint64_t ped, uint64_t tid, Common::List<EventDescriptorItem>* list)
    {
        add_process_to_event_list({ped, tid}, list);
        Task::Manager::instance().get_ed(ped, tid)->second.waitListElements.push_back(list);

    }

    void EventDispatcher::remove_process_from_listener_lists(uint64_t ped, uint64_t tid)
    {
        //SpinlockAcquirer acquirer(Task::TaskManager::get_spinlock());
        auto descriptor = Task::Manager::instance().get_ed(ped, tid);
        for (auto it = descriptor->second.listenerListElements.begin(); it!= descriptor->second.listenerListElements.end(); ++it)
        {
            for (auto listit = (*it)->begin(); listit!=(*it)->end(); ++listit)
            {
                if (listit->tid == tid) { (*it)->erase(listit); break; }
            }
        }
    }

    void EventDispatcher::remove_process_from_wait_lists(uint64_t ped, uint64_t tid)
    {
        //SpinlockAcquirer acquirer(Task::TaskManager::get_spinlock());
        auto descriptor = Task::Manager::instance().get_ed(ped, tid);
        for (auto it = descriptor->second.waitListElements.begin(); it!=descriptor->second.waitListElements.end(); ++it)
        {
            for (auto listit = (*it)->begin(); listit != (*it)->end(); ++listit)
            {
                if (listit->tid == tid) { (*it)->erase(listit); break; }
            }
        }
    }

    void EventDispatcher::wake_processes(Common::List<EventDescriptorItem>* list)
    {
        // We need to make a copy of the list as we are removing items from the original.
        auto listCopy = *list;
        for (auto it = listCopy.begin(); it != listCopy.end(); ++it)
        {
            remove_process_from_wait_lists((*it).processEventDescriptor, (*it).tid);
            Task::Manager::instance().unblock((*it).tid);
        }
        Task::Manager::instance().refresh();
    }

    uint64_t EventDispatcher::register_event_listener(uint64_t tid, char* event, void* param)
    { 
        auto str = Common::HashableString(event);
        
        auto eventSender = EventDispatcher::instance().m_eventSenderHashmap.find(str);
        if (!(eventSender->first == str))
            return -1;
        auto ed = Task::Manager::instance().add_ed((void*)eventSender->second, param, tid);
        eventSender->second->register_event_listener(ed, tid); 
        return ed;
    }

    void EventDispatcher::dispatch_event(EventMessage event, EventDescriptorItem processed)
    {
        auto ped = Task::Manager::instance().get_ed(processed.processEventDescriptor, processed.tid);
        ped->second.queue.push_back(event);
    }

    void EventDispatcher::dispatch_event(EventMessage event, Common::List<EventDescriptorItem>* processeds)
    {
        for (auto it=processeds->begin(); it != processeds->end(); ++it)
        {
            dispatch_event(event, *it);
        }
    }

    void EventDispatcher::block_event_listen(uint64_t tid, uint64_t ed)
    {
        auto ped = Task::Manager::instance().get_ed(ed, tid);
        auto sender = (EventSender*)(ped->second.eventSender);
        if (ped->second.queue.size() != 0) {
            return;
        }
        Task::Manager::instance().about_to_block();
        sender->block_event_listen(ed, tid);
        Task::Manager::instance().block();
    }

    uint64_t EventDispatcher::deregister_event_listener(uint64_t tid, uint64_t ed)
    {
        EventDispatcher::instance().remove_process_from_listener_lists(ed, tid);
        EventDispatcher::instance().remove_process_from_wait_lists(ed, tid);
        Task::Manager::instance().remove_ed(ed, tid);
        return 1;
    }

    uint64_t EventDispatcher::read_from_event_queue(uint64_t tid, uint64_t ed)
    {
        auto ped = Task::Manager::instance().get_ed(ed, tid);
        if (ped->second.queue.begin() == ped->second.queue.end())
        { 
            return 0;
        }
        auto ret = *(ped->second.queue.begin());
        ped->second.queue.erase(ped->second.queue.begin());
        return (uint64_t)ret.message;
    }
}
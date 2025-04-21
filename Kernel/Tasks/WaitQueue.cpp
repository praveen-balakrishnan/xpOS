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

#include "Tasks/TaskManager.h"
#include "Tasks/WaitQueue.h"

namespace Task
{

WaitQueue::Item WaitQueue::add_to_queue()
{
    auto tid = Manager::instance().get_current_tid();
    Manager::instance().about_to_block();
    LockAcquirer acquirer(m_lock);
    m_queue.push_back(tid);
    return --m_queue.end();
}

int WaitQueue::wake_queue()
{
    LockAcquirer acquirer(m_lock);
    for (auto tid : m_queue)
        Manager::instance().unblock(tid);
    
    return m_queue.size();
}

int WaitQueue::wake_one()
{
    LockAcquirer acquirer(m_lock);
    auto it = m_queue.begin();

    if (it == m_queue.end())
        return 0;
    
    Manager::instance().unblock(*it);
    return 1;
}

void WaitQueue::remove_from_queue(Item item)
{
    LockAcquirer acquirer(m_lock);
    m_queue.erase(item);
}

}
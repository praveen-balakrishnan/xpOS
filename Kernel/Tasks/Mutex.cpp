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

#include "Tasks/Mutex.h"

Mutex::Mutex()
    : m_acquired(false) {}

void Mutex::acquire()
{
    if (!Task::Manager::is_executing())
        return;
    m_lock.acquire();

    if (m_acquired) {
        // If the mutex is already acquired, we add the current task to a wait list and sleep to avoid busy-waiting.
        Task::Manager::instance().about_to_block();
        m_waitingTasks.push_back(Task::Manager::instance().get_current_tid());
        m_lock.release();
        Task::Manager::instance().block();
    } else {
        m_acquired = true;
        m_lock.release();
    }
}

void Mutex::release()
{
    if (!Task::Manager::is_executing())
        return;
    
    m_lock.acquire();

    auto it = m_waitingTasks.begin();
    if (it == m_waitingTasks.end()) {
        // There are no tasks trying to acquire the mutex, it can be released.
        m_acquired = false;
        m_lock.release();
    } else {
        // Wake the first task on the wait-list who now holds the mutex
        Task::Manager::instance().unblock(*it);
        m_waitingTasks.erase(it);
        m_lock.release();
        Task::Manager::instance().refresh();
    }
}
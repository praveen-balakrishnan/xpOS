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

#ifndef MUTEX_H
#define MUTEX_H

#include "Tasks/TaskManager.h"

class Mutex
{
    Common::List<Task::TaskID> m_waitingTasks;
    bool m_acquired;
    Spinlock m_lock;
public:
    Mutex();
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
    void acquire();
    void release();
    bool is_acquired()
    {
        return m_acquired;
    }
};

#endif
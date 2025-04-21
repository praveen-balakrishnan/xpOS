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

#ifndef WAITQUEUE_H
#define WAITQUEUE_H

#include "Common/List.h"
#include "Tasks/Spinlock.h"

using TaskID = uint64_t;

namespace Task
{

/**
 * Tasks can wait on a WaitQueue and can later be woken
 * up one at a time or all together.
 */
class WaitQueue
{    
public:
    using Item = Common::List<TaskID>::Iterator;
    Item add_to_queue();

    int wake_queue();

    int wake_one();

    void remove_from_queue(Item item);

    bool empty()
    {
        return m_queue.size() == 0;
    }

private:
    Spinlock m_lock;
    Common::List<TaskID> m_queue;
};

}

#endif
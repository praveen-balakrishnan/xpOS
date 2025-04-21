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

#include "Arch/IO/PIT.h"
#include "Tasks/MasterScheduler.h"
#include "Tasks/TaskManager.h"

#include "x86_64.h"

namespace Task
{

TaskID MasterScheduler::get_next_task()
{
    auto currentTime = X86_64::ProgrammableIntervalTimer::time_since_boot();
    auto currentTaskId = Manager::instance().get_current_tid();
    if (currentTaskId) {
        auto currentTask = Manager::instance().get_current_task();
        auto groupId = currentTask->groupId;
        auto group = Manager::instance().get_group_from_gid(groupId);

        if (group->readyTasks.size() > 0) {
            if (currentTime < m_groupSliceEndTime) {
                // We still have our time slice, so schedule the next thread.
                auto sliceTime = m_groupSliceEndTime - m_groupSliceStartTime;

                int taskNum = ((currentTime - m_groupSliceStartTime) * group->readyTasks.size()) / sliceTime;
                auto taskIt = group->readyTasks.begin();
                for (; taskNum > 0; taskNum--)
                    taskIt++;
                
                return *taskIt;
            } else {
                // No threads, so add our group to be scheduled later.
                add_group_to_queue(currentTask->groupId);
            }
        } else {
            group->scheduled = false;
        }
    }

    m_groupSliceStartTime = X86_64::ProgrammableIntervalTimer::time_since_boot();
    m_groupSliceEndTime = m_groupSliceStartTime + fixedTimeslice;
    // Each queue pre-empts the previous queue.
    GroupID gid = 0;
    if (m_queue0.size()) {
        gid = m_queue0.top().groupId;
        m_queue0.pop();
    } else if (m_queue1.size()) {
        gid = m_queue1.top().groupId;
        m_queue1.pop();
    } else if (m_queue2.size()) {
        gid = m_queue2.get_next();
    }

    if (gid)
        return *Manager::instance().get_group_from_gid(gid)->readyTasks.begin();
    return 0;
}


/*TaskID MasterScheduler::get_next_task(Spinlock& taskSpinlock)
{
    auto task = get_next_task();
    
    while (!task) {
        // If no tasks are runnable, we enable interrupts as this is the only way a task can become runnable.
        taskSpinlock.release();
        X86_64::sti();
        X86_64::hlt();
        X86_64::cli();
        taskSpinlock.acquire();
        task = get_next_task();
    }

    return task;
}*/

void MasterScheduler::deschedule_current_task()
{
    
}

void MasterScheduler::schedule_group(GroupID gid)
{
    add_group_to_queue(gid);
}

void MasterScheduler::add_group_to_queue(GroupID gid)
{
    auto group = Manager::instance().get_group_from_gid(gid);
    group->scheduled = true;
    auto priority = group->priority;
    auto queue = priority / 256;
    auto queuePriority = priority % 256;
    switch (queue) {
    case 0:
        m_queue0.push({gid, queuePriority});
        break;
    case 1:
        m_queue1.push({gid, queuePriority});
        break;
    case 2:
        m_queue2.add(gid, queuePriority);
        break;
    default:
        break;
    }
}

}
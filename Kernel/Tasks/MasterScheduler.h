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

#ifndef MASTERSCHEDULER_H
#define MASTERSCHEDULER_H

#include "Common/PriorityQueue.h"
#include "Tasks/Scheduler.h"
#include "Tasks/Task.h"

#define VARFREQ_CIRCULAR_BUFFER_SIZE 256

namespace Task
{
    struct TaskPriorityItem
    {
        GroupID groupId;
        int priority;

        bool operator==(const TaskPriorityItem& rhs) const = default;

        bool operator!=(const TaskPriorityItem& rhs) const = default;

        bool operator<(const TaskPriorityItem& rhs) const
        {
            return priority > rhs.priority;
        }

        bool operator<=(const TaskPriorityItem& rhs) const
        {
            return priority > rhs.priority;
        }

        bool operator>(const TaskPriorityItem& rhs) const
        {
            return priority < rhs.priority;
        }

        bool operator>=(const TaskPriorityItem& rhs) const
        {
            return priority <= rhs.priority;
        }
    };

    struct MasterSchedulerTaskInfo
    {
        uint64_t queue = 0;
        uint64_t priority = 0;
    };

    /*
        MasterScheduler (meta-scheduler)
        
        All schedulers pre-empt schedulers with a lower scheduler priority.
        Scheduler 0 (highest priority) : highest priority task runs first, used for very low latency - IRQ tasklets, device drivers etc.
        Scheduler 1 : highest priority task runs first, used for low latency - GUI updates, network stack processing etc.
        Scheduler 2 (lowest priority) : variable frequency fixed timeslice, used for normal tasks - userspace tasks
    */

    class MasterScheduler : public Scheduler
    {
    public:
        void schedule_group(GroupID gid);
        void deschedule_current_task();
        TaskID get_next_task(Spinlock& schedulerSpinlock);
    private:
        // The VariableFrequencyBuffer maintains tasks in a circular buffer, with lower priority tasks occuring more sparsely.
        class VariableFrequencyBuffer
        {
        private:
            Common::List<GroupID> buffer[VARFREQ_CIRCULAR_BUFFER_SIZE];
            uint64_t m_currentList = 0;
            uint64_t m_size = 0;
        public:
            VariableFrequencyBuffer() {}
            void add(GroupID gid, uint64_t priority)
            {
                buffer[(m_currentList + priority) % VARFREQ_CIRCULAR_BUFFER_SIZE].push_back(gid);
                m_size++;
            }
            TaskID get_next()
            {
                if (!m_size)
                    return 0;
                
                // Keep moving to next task list in circular buffer until we have a non-empty task list.
                while (!buffer[m_currentList].size()) {
                    m_currentList++;
                    m_currentList %= VARFREQ_CIRCULAR_BUFFER_SIZE;
                }
                // Remove and return the first task on the list.
                auto ret = *buffer[m_currentList].begin();
                buffer[m_currentList].erase(buffer[m_currentList].begin());
                m_size--;
                return ret;
            }
            uint64_t size() { return m_size; }
        };

        TaskID get_next_task();
        void add_group_to_queue(GroupID gid);

        Common::PriorityQueue<TaskPriorityItem> m_queue0;
        Common::PriorityQueue<TaskPriorityItem> m_queue1;
        VariableFrequencyBuffer m_queue2;

        uint64_t m_groupSliceStartTime = 0;
        uint64_t m_groupSliceEndTime = 0;
        static constexpr uint64_t fixedTimeslice = 15;

    };
}

#endif
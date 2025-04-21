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

#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "Common/PriorityQueue.h"
#include "Tasks/Scheduler.h"
#include "Tasks/Task.h"
#include "x86_64.h"

namespace Task
{

struct TaskDescriptor
{
    ARC<Memory::RegionableVirtualAddressSpace> addressSpace;
    ARC<PipeTable> openPipes;
    void* entryPoint = nullptr;
    void* launchParam = nullptr;
    int taskPriority = 512;
    GroupID groupId = 0;
};

class Manager
{
    friend class Mutex;
public:
    void begin();

    /**
     * Flags the current task to block on next block() call.
     */
    void about_to_block();

    /**
     * Blocks if the block flag is set. The block flag can be set
     * using about_to_block() and can be cleared by waking the task.
     */
    void block();

    /**
     * Flags a blocked task to unblock on the next task refresh.
     */
    void unblock(TaskID tid);

    Task* get_task_from_tid(TaskID tid)
    {
        return m_taskHashmap.find(tid)->second;
    }

    Group* get_group_from_gid(GroupID gid)
    {
        return m_groupHashmap.find(gid)->second;
    }

    TaskID launch_task(TaskDescriptor taskDescriptor);
    TaskID launch_kernel_process(void* entryPoint, void* launchParam, int taskPriority = 512);
    TaskID launch_thread(void* entryPoint, void* launchParam);

    TaskID get_current_tid()
    {
        return m_currentTask;
    }

    Task* get_current_task()
    {
        return m_currentTask != 0 ? get_task_from_tid(m_currentTask) : nullptr;
    }

    Group* get_current_group()
    {
        return get_group_from_gid(get_current_task()->groupId);
    }

    /**
     * Refreshes the scheduler to execute the most appropriate task.
     */
    void refresh();
    
    static bool is_executing();

    static void task_cleanup();

    void terminate_task();
    /**
     * Put current task to sleep until a later time. 
     */ 
    void sleep_until(uint64_t time);
    /**
     * Put current task to sleep for a given duration.
     */
    static void sleep_for(uint64_t duration);

    void initialise(Scheduler* scheduler);
    
    static Manager& instance()
    {
        static Manager instance;
        return instance;
    }

private:
    Manager() {}
    void lockless_unblock(TaskID tid);

    Scheduler* m_scheduler;
    static bool m_isActive;
    bool m_schedulerChangingTask = false;
    std::size_t m_taskCount = 0;
    std::size_t m_groupCount = 0;

    Spinlock m_sleepingTasksLock;
    Spinlock m_schedulerLock;

    TaskID m_currentTask = 0;
    Common::Hashmap<TaskID, Task*> m_taskHashmap;
    Common::Hashmap<GroupID, Group*> m_groupHashmap;

    struct SleepingTask
    {
        TaskID tid;
        uint64_t wakeTime;
        
        bool operator==(const SleepingTask& rhs) const = default;
        
        bool operator!=(const SleepingTask& rhs) const = default;

        bool operator<(const SleepingTask& rhs) const
        {
            return wakeTime < rhs.wakeTime;
        }

        bool operator<=(const SleepingTask& rhs) const
        {
            return wakeTime <= rhs.wakeTime;
        }

        bool operator>(const SleepingTask& rhs) const
        {
            return wakeTime > rhs.wakeTime;
        }

        bool operator>=(const SleepingTask& rhs) const
        {
            return wakeTime >= rhs.wakeTime;
        }
    };

    Common::PriorityQueue<SleepingTask> m_sleepingTasks;
    
    static constexpr uint64_t START_OF_PROCESS_KSTACKS = 0xFFFFCF8000000000;
};
}

#endif
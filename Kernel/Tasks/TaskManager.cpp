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

#include "Arch/CPU.h"
#include "Arch/IO/PIT.h"
#include "Arch/TSS.h"
#include "Memory/AddressSpace.h"
#include "Memory/MemoryManager.h"
#include "Tasks/Scheduler.h"
#include "Tasks/Task.h"
#include "Tasks/TaskManager.h"
#include "x86_64.h"

namespace Task
{

extern "C" void enable_syscall_sysret();

extern "C" void switch_to_not_started_task(void** kstack, void* tlTable, void* entryPoint, void** kstackOld, void* cleanUpCallback, void* launchParam);
extern "C" void switch_to_ready_task(void** kstack, void* tlTable, void** kstackOld);

bool Manager::m_isActive = false;

void Manager::initialise(Scheduler* scheduler)
{
    enable_syscall_sysret();
    constexpr int sizeOfStack = Memory::PAGE_4KiB * 32;
    auto stackBottom = START_OF_PROCESS_KSTACKS;
    // Allocate a safe kernel stack for processes to use.
    for (int i = 0; i < sizeOfStack; i += Memory::PAGE_4KiB) {
        Memory::VirtualMemoryAllocationRequest request(Memory::VirtualAddress(stackBottom + i), true);
        Memory::Manager::instance().alloc_page(request);
    }

    CPU::set_ring_stack_pointer(Memory::VirtualAddress(START_OF_PROCESS_KSTACKS + sizeOfStack).get(), CPU::Ring::KERNEL);
    CPU::refresh_task_state_segment();
    m_scheduler = scheduler;
}

TaskID Manager::launch_task(TaskDescriptor taskDescriptor)
{
    auto task = new Task(taskDescriptor.addressSpace, taskDescriptor.openPipes, taskDescriptor.taskPriority);
    task->tid = ++m_taskCount;
    /**
     * Queue 0: 0-255
     * Queue 1: 256-511
     * Queue 2: 512-767 
     */

    // At the moment, task stacks are allocated contiguously so we can allocate the stack at a position based on the task id.
    /// FIXME: We should have 8MiB stacks in some region of the address space, and the page fault handler should allocate pages for this.
    constexpr int sizeOfStack = Memory::PAGE_4KiB * 32;
    auto stackBottom = (START_OF_PROCESS_KSTACKS + sizeOfStack * (task->tid));
    for (int i = 0; i < sizeOfStack; i += Memory::PAGE_4KiB)
        Memory::Manager::instance().alloc_page(Memory::VirtualMemoryAllocationRequest(Memory::VirtualAddress(stackBottom + i), true));

    // We need to map a safe kernel stack and the task's kernel stack into the task's address space.
    for (int i = 0; i < sizeOfStack; i += Memory::PAGE_4KiB) {
        Memory::VirtualMemoryMapRequest memoryMapRequest = {
            .physicalAddress = Memory::Manager::instance().get_physical_address(START_OF_PROCESS_KSTACKS + i),
            .virtualAddress = Memory::VirtualAddress(START_OF_PROCESS_KSTACKS + i),
            .allowWrite = true
        };
        Memory::Manager::instance().request_virtual_map(memoryMapRequest, *task->tlTable);
        memoryMapRequest = {
            .physicalAddress = Memory::Manager::instance().get_physical_address(stackBottom + i),
            .virtualAddress = Memory::VirtualAddress(stackBottom + i),
            .allowWrite = true
        };
        Memory::Manager::instance().request_virtual_map(memoryMapRequest, *task->tlTable);
    }
    
    task->kstack = reinterpret_cast<void*>(stackBottom + sizeOfStack);
    task->groupId = taskDescriptor.groupId;
    task->entryPoint = taskDescriptor.entryPoint;
    task->launchParam = taskDescriptor.launchParam;

    {
        LockAcquirer acquirer(m_schedulerLock);
        
        Group* group;
        if (task->groupId == 0) {
            task->groupId = ++m_groupCount;
            group = new Group {
                .groupId = task->groupId,
                .priority = task->priority
            };
            m_groupHashmap.insert({task->groupId, group});
        } else {
            group = get_group_from_gid(task->groupId);
        }

        group->readyTasks.push_back(task->tid);

        m_taskHashmap.insert({task->tid, task});
        if (!group->scheduled)
            m_scheduler->schedule_group(task->groupId);
    }
    return task->tid;
}

TaskID Manager::launch_kernel_process(void* entryPoint, void* launchParam, int taskPriority)
{
    auto& mainAddrSpace = Memory::Manager::instance().get_main_address_space();
    return launch_task({
        .addressSpace = ARC<Memory::RegionableVirtualAddressSpace>(mainAddrSpace),
        .openPipes = ARC<PipeTable>(PipeTable()),
        .entryPoint = entryPoint,
        .launchParam = launchParam,
        .taskPriority = taskPriority
    });
}

TaskID Manager::launch_thread(void* entryPoint, void* launchParam)
{
    auto currentTask = get_current_task();
    
    return launch_task({
        .addressSpace = currentTask->tlTable,
        .openPipes = currentTask->openPipes,
        .entryPoint = entryPoint,
        .launchParam = launchParam,
        .groupId = currentTask->groupId
    });
}

void Manager::sleep_for(uint64_t duration)
{
    auto currentTime = X86_64::ProgrammableIntervalTimer::time_since_boot();
    Manager::instance().sleep_until(currentTime + duration);
}

void Manager::sleep_until(uint64_t time)
{
    auto currentTime = X86_64::ProgrammableIntervalTimer::time_since_boot();
    if (currentTime >= time) {
        return;
    }

    SleepingTask sleepingTask = {
        .tid = get_current_tid(),
        .wakeTime = time
    };


    about_to_block();
    // Add sleeping task to list of sleeping tasks that is checked on every timer interrupt.
    {
        LockAcquirer acquirer(m_sleepingTasksLock);
        m_sleepingTasks.push(sleepingTask);
    }
    block();
}

void Manager::refresh()
{
    X86_64::cli();
    auto currentTime = X86_64::ProgrammableIntervalTimer::time_since_boot();

    {
        // Check if there are any task sleep timers have expired.
        LockAcquirer acquirer(Manager::instance().m_sleepingTasksLock);
        while (Manager::instance().m_sleepingTasks.size() > 0) {
            auto task = Manager::instance().m_sleepingTasks.top();
            if (currentTime >= task.wakeTime) {
                Manager::instance().m_sleepingTasks.pop();
                Manager::instance().lockless_unblock(task.tid);
            }
            else {
                break;
            }
        }
    }
    Task* lastTask = nullptr;
    {
        LockAcquirer acquirer(Manager::instance().m_schedulerLock);
        if (Manager::instance().m_schedulerChangingTask)
            return;
        
        if (Manager::instance().get_current_tid()) 
            lastTask = Manager::instance().get_current_task();
        
        
        Manager::instance().m_schedulerChangingTask = true;
        m_currentTask = Manager::instance().m_scheduler->get_next_task();

        while (!m_currentTask) {
            // If no tasks are runnable, we enable interrupts as this is the only way a task can become runnable.
            m_schedulerLock.release();
            X86_64::sti();
            X86_64::hlt();
            X86_64::cli();
            m_schedulerLock.acquire();
            m_currentTask = Manager::instance().m_scheduler->get_next_task();
        }

        Manager::instance().m_schedulerChangingTask = false;
    }

    void* tlTable;
    Task* task;

    task = get_current_task();
    
    tlTable = (*task->tlTable).get_physical_address().get();

    Memory::VirtualAddress stackPointer = START_OF_PROCESS_KSTACKS + Memory::PAGE_4KiB * task->tid + Memory::PAGE_4KiB;
    CPU::set_ring_stack_pointer(stackPointer.get(), CPU::Ring::KERNEL);

    if (task->state == Task::State::NOT_STARTED) {
        task->state = Task::State::READY;
        switch_to_not_started_task(&task->kstack, tlTable, task->entryPoint, lastTask != nullptr ? &lastTask->kstack : 0, (void*)task_cleanup, task->launchParam);
    } else {
        switch_to_ready_task(&task->kstack, tlTable, lastTask != nullptr ? &lastTask->kstack : 0);
    }
}

void Manager::begin()
{
    m_isActive = true;
    refresh();
}

bool Manager::is_executing()
{
    return m_isActive;
}

void Manager::about_to_block()
{
    auto* task = get_current_task();
    LockAcquirer l(task->stateLock);
    task->blockFlag = true;
}

void Manager::block()
{
    auto* currentTask = get_current_task();
    {
        LockAcquirer l(currentTask->stateLock);
        if (!currentTask)
            return;
        
        // We have already been woken up before we have had a chance
        // to block, so just return immediately.
        if (!currentTask->blockFlag)
            return;
        
        currentTask->state = Task::State::WAIT;
        {
            LockAcquirer acquirer(m_schedulerLock);
            auto group = get_group_from_gid(currentTask->groupId);
            for (auto it = group->readyTasks.begin(); it != group->readyTasks.end(); it++) {
                if (*it == currentTask->tid) {
                    group->readyTasks.erase(it);
                    break;
                }
            }
            m_scheduler->deschedule_current_task();
        }
    }
    refresh();
}

void Manager::unblock(TaskID tid)
{
    auto* task = get_task_from_tid(tid);
    LockAcquirer l(task->stateLock);
    task->blockFlag = false;

    if (task->state == Task::State::READY) 
        return;
    
    task->state = Task::State::READY;
    LockAcquirer acquirer(m_schedulerLock);
    auto* group = get_group_from_gid(task->groupId);
    group->readyTasks.push_back(tid);
    if (!group->scheduled)
        m_scheduler->schedule_group(task->groupId);
    
}

void Manager::lockless_unblock(TaskID tid)
{
    auto* task = get_task_from_tid(tid);
    LockAcquirer l(task->stateLock);
    task->blockFlag = false;

    if (task->state == Task::State::READY)
        return;
    
    task->state = Task::State::READY;
    auto* group = get_group_from_gid(task->groupId);
    group->readyTasks.push_back(tid);
    if (!group->scheduled)
        m_scheduler->schedule_group(task->groupId);
}

void Manager::task_cleanup()
{
    Manager::instance().terminate_task();
}

void Manager::terminate_task()
{
    /// FIXME: We should free up resources here.
    about_to_block();
    block();
}

}

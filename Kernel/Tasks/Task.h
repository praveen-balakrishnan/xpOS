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

#ifndef TASK_H
#define TASK_H

#include <utility>

#include "Common/Hashmap.h"
#include "Common/List.h"
#include "Common/Optional.h"
#include "Common/ReferenceCounting.h"
#include "Memory/AddressSpace.h"
#include "Memory/MemoryManager.h"
#include "Tasks/WaitQueue.h"

namespace Pipes
{
    class Pipe;
}

namespace Task
{

static constexpr uint64_t SIZE_OF_IRETQ_STACK = 20*8;
using TaskID = std::size_t;
using GroupID = std::size_t;

template <class T>
class DescriptorTable
{
public:
    auto insert(T descriptor)
    {
        m_hashmap.insert({m_nextAvailID, descriptor});
        return m_hashmap.find(m_nextAvailID++);
    }
    void remove(uint64_t id)
    {
        m_hashmap.erase(id);
    }
    auto get(uint64_t descriptorID)
    {
        return m_hashmap.find(descriptorID);
    }
private:
    uint64_t m_nextAvailID = 0;
    Common::Hashmap<uint64_t, T> m_hashmap;
};

class PipeTable
{
public:
    auto insert(Pipes::Pipe* pipe)
    {
        auto pair = m_hashmap.insert({m_nextAvailID++, pipe});
        return pair.first->first;
    }

    void remove(uint64_t id)
    {
        m_hashmap.erase(id);
    }

    Common::Optional<Pipes::Pipe*> get(uint64_t pipeId)
    {
        auto it = m_hashmap.find(pipeId);
        if (it != m_hashmap.end())
            return it->second;
        else
            return Common::Nullopt;
    }

private:
    uint64_t m_nextAvailID = 3;
    Common::Hashmap<uint64_t, Pipes::Pipe*> m_hashmap;
};

template <typename T>
using ARC = Common::AutomaticReferenceCountable<T>;

/**
 * Represents a task that can be scheduled for execution or is
 * currently executing.
 */
struct Task
{
    friend class Manager;
    enum class State
    {
        NOT_STARTED,
        READY,
        WAIT
    };

    ARC<Memory::RegionableVirtualAddressSpace> tlTable;
    ARC<PipeTable> openPipes;

    void* stack;
    void* kstack;
    uint64_t tid;
    int priority;
    void* entryPoint;
    void* launchParam;
    State state = State::NOT_STARTED;
    GroupID groupId;
    bool blockFlag = false;

    Spinlock stateLock;

private: 
    Task(
        ARC<Memory::RegionableVirtualAddressSpace> rvas,
        ARC<PipeTable> pipes,
        uint64_t taskPriority
    )
        : tlTable(rvas)
        , openPipes(pipes)
        , priority(taskPriority)
    {}
};

struct Group
{
    GroupID groupId;
    int priority;
    bool scheduled = false;
    Common::List<TaskID> readyTasks;
    
    Common::Hashmap<uintptr_t, WaitQueue*> futexWaitQueues;
    Spinlock futexLock;
};

}

#endif
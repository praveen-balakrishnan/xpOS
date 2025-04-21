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
#include "API/Syscall.h"
#include "API/Network.h"
#include "API/Pipes.h"
#include "API/Event.h"
#include "Networking/NetworkSocket.h"
#include "Pipes/EventListener.h"
#include "Pipes/LocalSocket.h"
#include "Syscalls/SyscallHandler.h"
#include "Tasks/TaskManager.h"

#include "print.h"

/// FIXME: Validate these syscalls so that everything is checked

uint64_t sleep_for_syscall(uint64_t duration)
{
    Task::Manager::instance().sleep_for(duration);
    return 0;
}

uint64_t vmmap_syscall(uint64_t* address, uint64_t size, uint64_t hint, uint64_t flags)
{
    if (!(flags & 0x08))
        return -1;

    auto currentTask = Task::Manager::instance().get_current_task();
    auto region = (*currentTask->tlTable).acquire_available_region(size);
    auto regionBegin = reinterpret_cast<uint64_t>(region.start);
    for (uint64_t allocateSpace = regionBegin; allocateSpace < regionBegin + size; allocateSpace += Memory::PAGE_4KiB) {
        Memory::VirtualMemoryAllocationRequest request(allocateSpace, true, true);
        Memory::Manager::instance().alloc_page(request, *currentTask->tlTable);
    }
    *address = regionBegin;
    return 0;
}


uint64_t vmumap_syscall(uint64_t addr, uint64_t size)
{
    auto currentTask = Task::Manager::instance().get_current_task();
    auto region = (*currentTask->tlTable).extract_region(reinterpret_cast<void*>(addr));
    auto regionBegin = reinterpret_cast<uint64_t>(region.start);
    for (uint64_t deallocSpace = regionBegin; deallocSpace < regionBegin + size; deallocSpace += Memory::PAGE_4KiB) {
        Memory::VirtualMemoryFreeRequest request(deallocSpace);
        Memory::Manager::instance().free_page(request, *currentTask->tlTable);
    }
    return 0;
}

uint64_t popen_syscall(uint64_t dev, uint64_t with, uint64_t flags)
{
    auto task = Task::Manager::instance().get_current_task();
    Pipes::Pipe* pipe = new Pipes::Pipe(reinterpret_cast<const char*>(dev), const_cast<void*>(reinterpret_cast<const void*>(with)), flags);
    return (*task->openPipes).insert(pipe);
}

uint64_t pread_syscall(uint64_t pd, uint64_t buf, uint64_t count)
{
    auto task = Task::Manager::instance().get_current_task();
    auto optPipe = (*task->openPipes).get(pd);
    if (optPipe.has_value())
        return (*optPipe)->read(count, reinterpret_cast<void*>(buf));
    else
        return 0;
}

uint64_t pwrite_syscall(uint64_t pd, uint64_t buf, uint64_t count)
{
    auto task = Task::Manager::instance().get_current_task();
    auto optPipe = (*task->openPipes).get(pd);

    if (optPipe.has_value())
        return (*optPipe)->write(count, reinterpret_cast<void*>(buf));
    else
        return 0;
}

uint64_t pclose_syscall(uint64_t pd)
{
    auto task = Task::Manager::instance().get_current_task();
    auto optPipe = (*task->openPipes).get(pd);
    if (optPipe.has_value()) {
        delete *optPipe;
        (*task->openPipes).remove(pd);
    }
    return 0;
}

long pseek_syscall(uint64_t pd, long offset, int type)
{
    auto task = Task::Manager::instance().get_current_task();
    auto optPipe = (*task->openPipes).get(pd);
    if (optPipe.has_value())
        return (*optPipe)->seek(offset, static_cast<xpOS::API::Pipes::SeekType>(type));
    else
        return 0;
}

uint64_t pinfo_syscall(uint64_t pd, uint64_t usrptr)
{
    auto* pipeInfo = reinterpret_cast<xpOS::API::Pipes::PipeInfo*>(usrptr);
    auto task = Task::Manager::instance().get_current_task();
    auto optPipe = (*task->openPipes).get(pd);
    if (optPipe.has_value()) {
        *pipeInfo = (*optPipe)->info();
        return 0;
    } else {
        return -1;
    }
}

uint64_t elistener_add_syscall(uint64_t listenerpd, uint64_t targetpd, uint64_t eventMask)
{
    auto task = Task::Manager::instance().get_current_task();
    auto optPipe = (*task->openPipes).get(listenerpd);
    if (!optPipe.has_value())
        return 0;
    
    auto optTargetPipe = (*task->openPipes).get(targetpd);
    if (!optTargetPipe.has_value())
        return 0;
    
    Pipes::EventListener::add(**optPipe, *optTargetPipe, targetpd, eventMask);
    return 1;
}

uint64_t elistener_remove_syscall(uint64_t listenerpd, uint64_t targetpd, uint64_t eventMask)
{
    auto task = Task::Manager::instance().get_current_task();
    auto optPipe = (*task->openPipes).get(listenerpd);
    if (!optPipe.has_value())
        return 0;
    
    auto optTargetPipe = (*task->openPipes).get(targetpd);
    if (!optTargetPipe.has_value())
        return 0;
    Pipes::EventListener::remove(**optPipe, *optTargetPipe, targetpd, eventMask);
    return 1;
}

uint64_t nsock_bind(uint64_t pd, Networking::Endpoint endpoint)
{
    auto task = Task::Manager::instance().get_current_task();
    auto optPipe = (*task->openPipes).get(pd);
    if (!optPipe.has_value())
        return 0;
    
    return Networking::NetworkSocket::bind(**optPipe, endpoint);
}

uint64_t nsock_connect(uint64_t pd, Networking::Endpoint endpoint)
{
    auto task = Task::Manager::instance().get_current_task();
    auto optPipe = (*task->openPipes).get(pd);
    if (!optPipe.has_value())
        return 0;
    
    return Networking::NetworkSocket::connect(**optPipe, endpoint);
}

uint64_t lsock_accept(uint64_t pd)
{
    auto task = Task::Manager::instance().get_current_task();

    auto optPipe = (*task->openPipes).get(pd);
    if (!optPipe.has_value())
        return 0;

    auto* newPipe = new Pipes::Pipe("socket::local", nullptr, static_cast<Pipes::EventTypes>(Sockets::LocalSocket::Flags::NON_BLOCKING));

    auto didCreate = Sockets::LocalSocket::accept(**optPipe, *newPipe);
    if (!didCreate) {
        delete newPipe;
        return 0;
    } 
    return (*task->openPipes).insert(newPipe);
}

uint64_t lsock_connect(uint64_t pd, const char* id)
{
    auto task = Task::Manager::instance().get_current_task();

    auto optPipe = (*task->openPipes).get(pd);
    if (!optPipe.has_value())
        return 0;
    
    return Sockets::LocalSocket::connect(**optPipe, id);
}

uint64_t lsock_bind(uint64_t pd, const char* id)
{
    auto task = Task::Manager::instance().get_current_task();

    auto optPipe = (*task->openPipes).get(pd);
    if (!optPipe.has_value())
        return 0;
    
    return Sockets::LocalSocket::bind(**optPipe, id);
}

uint64_t lsock_listen(uint64_t pd)
{
    auto task = Task::Manager::instance().get_current_task();

    auto optPipe = (*task->openPipes).get(pd);
    if (!optPipe.has_value())
        return 0;
    
    return Sockets::LocalSocket::listen(**optPipe);
}

struct ThreadLaunchDescriptor
{
    void* entryPoint;
    void* stack;
};

extern "C" void start_userspace_task_sysret(uint64_t entryPoint, uint64_t userStackPointer);

uint64_t thread_start_helper(void* threadLaunchDescriptor)
{
    auto* tld = reinterpret_cast<ThreadLaunchDescriptor*>(threadLaunchDescriptor);
    auto* entryPoint = tld->entryPoint;
    auto* stack = tld->stack;
    delete tld;
    start_userspace_task_sysret(reinterpret_cast<uint64_t>(entryPoint), reinterpret_cast<uint64_t>(stack));
    return 0;
}

uint64_t launch_thread(uint64_t entryPoint, uint64_t stack)
{
    auto* tld = new ThreadLaunchDescriptor {
        .entryPoint = reinterpret_cast<void*>(entryPoint),
        .stack = reinterpret_cast<void*>(stack)
    };
    
    return Task::Manager::instance().launch_thread((void*)&thread_start_helper, tld);
}

uint64_t futex_wake(uint64_t ptr)
{
    auto group = Task::Manager::instance().get_current_group();
    //auto* futex = reinterpret_cast<int*>(ptr);
    LockAcquirer l(group->futexLock);

    auto it = group->futexWaitQueues.find(reinterpret_cast<uintptr_t>(ptr));

    if (it == group->futexWaitQueues.end() || it->second->empty()) {
        return 0;
    }
    it->second->wake_one();

    return 1;
}

uint64_t futex_wait(uint64_t ptr, uint64_t exp)
{
    auto group = Task::Manager::instance().get_current_group();
    int* futex = reinterpret_cast<int*>(ptr);
    int expected = static_cast<int>(exp);
    
    group->futexLock.acquire();

    auto it = group->futexWaitQueues.find(reinterpret_cast<uintptr_t>(ptr));

    if (it == group->futexWaitQueues.end()) {
        auto* wq = new Task::WaitQueue;
        it = group->futexWaitQueues.insert({reinterpret_cast<uintptr_t>(ptr), wq}).first;
    }
    auto wqItem = it->second->add_to_queue();

    while (*futex == expected) {
        group->futexLock.release();
        Task::Manager::instance().block();
        group->futexLock.acquire();
    }

    it = group->futexWaitQueues.find(reinterpret_cast<uintptr_t>(ptr));
    it->second->remove_from_queue(wqItem);
    group->futexLock.release();

    return 0;
}

uint64_t printtoscreen_syscall(uint64_t arg1)
{
    char* str = (char*)arg1;
    printf(str);
    return 1;
}

uint64_t printtoscreennum_syscall(uint64_t arg1)
{
    printf(arg1);
    return 1;
}

uint64_t gettid_syscall()
{
    return Task::Manager::instance().get_current_tid();
}

uint64_t setfsbase_syscall(uint64_t arg1)
{
    static constexpr uint64_t FS_BASE_MSR =  0xC0000100;
    asm volatile("wrmsr" ::"a"(arg1 & 0xFFFFFFFF),
                 "d"((arg1 >> 32) & 0xFFFFFFFF), "c"(FS_BASE_MSR));
    return 0;
}

uint64_t boot_ticks_ms_syscall()
{
    return X86_64::ProgrammableIntervalTimer::instance().time_since_boot();
}

uint64_t Processes::syscall_handler(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    uint64_t rrax;
    asm volatile("mov %%rax, %0" : "=r" (rrax));
    switch (rrax)
    {
    case SYSCALL_DPRINTF:
        return printtoscreen_syscall(arg1);
    case SYSCALL_VMMAP:
        return vmmap_syscall(reinterpret_cast<uint64_t*>(arg1), arg2, arg3, arg4);
    case SYSCALL_VMUNMAP:
        return vmumap_syscall(arg1, arg2);
    case SYSCALL_POPEN:
        return popen_syscall(arg1, arg2, arg3);
    case SYSCALL_PREAD:
        return pread_syscall(arg1, arg2, arg3);
    case SYSCALL_PWRITE:
        return pwrite_syscall(arg1, arg2, arg3);
    case SYSCALL_PCLOSE:
        return pclose_syscall(arg1);
    case SYSCALL_SLEEP_FOR:
        return sleep_for_syscall(arg1);
    case SYSCALL_GETTID:
        return gettid_syscall();
    case SYSCALL_SETFSBASE:
        return setfsbase_syscall(arg1);
    case SYSCALL_PSEEK:
        return pseek_syscall(arg1, arg2, arg3);
    case SYSCALL_ELISTENER_ADD:
        return elistener_add_syscall(arg1, arg2, arg3);
    case SYSCALL_ELISTENER_REMOVE:
        return elistener_remove_syscall(arg1, arg2, arg3);
    case SYSCALL_LSOCK_ACCEPT:
        return lsock_accept(arg1);
    case SYSCALL_LSOCK_BIND:
        return lsock_bind(arg1, reinterpret_cast<const char*>(arg2));
    case SYSCALL_LSOCK_CONNECT:
        return lsock_connect(arg1, reinterpret_cast<const char *>(arg2));
    case SYSCALL_LSOCK_LISTEN:
        return lsock_listen(arg1);
    case SYSCALL_DNUMPRINT:
        return printtoscreennum_syscall(arg1);
    case SYSCALL_LAUNCH_THREAD:
        return launch_thread(arg1, arg2);
    case SYSCALL_FUTEX_WAIT:
        return futex_wait(arg1, arg2);
    case SYSCALL_FUTEX_WAKE:
        return futex_wake(arg1);
    case SYSCALL_PINFO:
        return pinfo_syscall(arg1, arg2);
    case SYSCALL_BOOTTICKS_MS:
        return boot_ticks_ms_syscall();
    }
    return 0;
}


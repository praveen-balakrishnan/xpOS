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

#include <cstdint>

#include "Tasks/Spinlock.h"
#include "print.h"
#include "x86_64.h"

uint64_t Spinlock::m_cli = 0;
bool Spinlock::m_shouldRestore = false;

// When acquiring spinlocks, interrupts on the local processor must be disabled to avoid a race condition.

void Spinlock::acquire()
{
    // We want to count the number of spinlocks acquired so that we only re-enable interrupts when all are released.
    push_cli();
    while (__atomic_test_and_set(&m_locked, __ATOMIC_SEQ_CST));
}

bool Spinlock::try_acquire()
{
    push_cli();
    if (__atomic_test_and_set(&m_locked, __ATOMIC_SEQ_CST))
    {
        // We could not acquire the spinlock.
        pop_cli();
        return false;
    }
    return true;
}

void Spinlock::release()
{
    __atomic_clear(&m_locked, __ATOMIC_SEQ_CST);
    pop_cli();
}

void Spinlock::push_cli()
{
    uint64_t rflags;
    rflags = X86_64::read_rflags();
    X86_64::cli();
    // If interrupts are enabled, store this so we can re-enable once we have released all spinlocks.
    if (Spinlock::m_cli++ == 0)
        Spinlock::m_shouldRestore = rflags & X86_64::INTERRUPT_FLAG;
}

void Spinlock::pop_cli()
{
    // If we have released all spinlocks and interrupts were previously enabled, re-enable interrupts.
    if (--Spinlock::m_cli == 0 && Spinlock::m_shouldRestore)
        X86_64::sti();
}

bool Spinlock::is_acquired()
{
    return m_locked;
}

/*SpinlockAcquirer::SpinlockAcquirer(Spinlock& lock)
    : m_lock(&lock)
{
    m_lock->acquire();
}

SpinlockAcquirer::SpinlockAcquirer(SpinlockAcquirer&& from)
    : m_lock(from.m_lock)
{
    from.m_lock = nullptr;
}

SpinlockAcquirer::~SpinlockAcquirer()
{
    if (m_lock && m_lock->is_acquired())
    {
        m_lock->release();
    }
}*/


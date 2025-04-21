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

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <cstdint>

class Spinlock
{
public:
    static uint64_t m_cli;
    static bool m_shouldRestore;
    volatile uint64_t m_locked = 0;
    void pop_cli();
    void push_cli();
public:
    void acquire();
    void release();
    bool try_acquire();
    bool is_acquired();
};

template <typename LockType>
class LockAcquirer
{
public:
    LockAcquirer(LockType& lock)
        : m_lock(&lock)
    {
        m_lock->acquire();
    }

    LockAcquirer(const LockAcquirer&) = delete;

    LockAcquirer& operator=(LockAcquirer&&) = delete;

    LockAcquirer(LockAcquirer&& from)
        : m_lock(from.m_lock)
    {
        from.m_lock = nullptr;
    }

    ~LockAcquirer()
    {
        if (m_lock && m_lock->is_acquired())
            m_lock->release();
    }
private:
    LockType* m_lock;
};

template<bool SpinlockEnable = false>
class ConditionalSpinlockAcquirer
{
public:
    ConditionalSpinlockAcquirer(Spinlock& lock)
        : m_lock(&lock)
    {
        if constexpr (SpinlockEnable)
            m_lock->acquire();
    }
    ConditionalSpinlockAcquirer(const ConditionalSpinlockAcquirer&) = delete;
    ConditionalSpinlockAcquirer& operator=(ConditionalSpinlockAcquirer&) = delete;
    ConditionalSpinlockAcquirer(ConditionalSpinlockAcquirer&& from) = delete;
    ~ConditionalSpinlockAcquirer()
    {
        if constexpr (SpinlockEnable) {
            if (m_lock && m_lock->is_acquired())
                m_lock->release();
        }
    }
private:
    Spinlock* m_lock;
};

#endif
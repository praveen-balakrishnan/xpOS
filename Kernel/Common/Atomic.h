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

#ifndef XPOS_COMMON_ATOMIC_H
#define XPOS_COMMON_ATOMIC_H

#include <concepts>
#include "Process/Spinlock.h"

namespace Common
{

template <std::integral T>
class Spinlocked
{
public:
    Spinlocked() = default;
    ~Spinlocked() = default;
    Spinlocked(const Spinlocked&) = delete;
    Spinlocked& operator=(const Spinlocked&) = delete;

    void lock()
    {
        m_lock.acquire();
    }
    void unlock()
    {
        m_lock.release();
    }
    operator T() const
    {
        LockAcquirer acquirer(m_lock);
        return m_contents;
    }

    constexpr Spinlocked(T t) : m_contents(t) {}

    T operator=(T i)
    {
        LockAcquirer acquirer(m_lock);
        m_contents.operator=(i);
        return i;
    }

    T operator++(int)
    {
        LockAcquirer acquirer(m_lock);
        return m_contents++;
    }

    T operator++()
    {
        LockAcquirer acquirer(m_lock);
        return ++m_contents;
    }

    T operator--(int)
    {
        LockAcquirer acquirer(m_lock);
        return m_contents--;
    }

    T operator--()
    {
        LockAcquirer acquirer(m_lock);
        return --m_contents;
    }

    T operator+=(T i)
    {
        LockAcquirer acquirer(m_lock);
        return m_contents += i;
    }

    T operator-=(T i)
    {
        LockAcquirer acquirer(m_lock);
        return m_contents -= i;
    }

private:
    T m_contents;
    Spinlock m_lock;
};

}

#endif
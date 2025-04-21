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

#include "guard-abi.h"

/**
 * gcc requires these to be implemented.
 * These are equivalent to a spinlock.
 */
namespace
{
    struct Guard {
        uint8_t initialised;
        uint32_t locked;

        void lock()
        {
            if (!__atomic_test_and_set(&locked, __ATOMIC_SEQ_CST)) 
            {
                return;
            }

            __builtin_trap();
        }

        void unlock()
        {
            __atomic_clear(&locked, __ATOMIC_SEQ_CST);
        }
    };

    static_assert(sizeof(Guard) == sizeof(uint64_t));

}

#ifdef __cplusplus
extern "C"
{
#endif

int __cxa_guard_acquire(uint64_t* ptr) {
	auto guard = reinterpret_cast<Guard*>(ptr);
	guard->lock();
    
	if (__atomic_load_n(&guard->initialised, __ATOMIC_RELAXED))
    {
		guard->unlock();
		return 0;
	}
    else
    {
		return 1;
	}
}

void __cxa_guard_release(uint64_t* ptr) {
	auto guard = reinterpret_cast<Guard*>(ptr);
    
	__atomic_store_n(&guard->initialised, 1, __ATOMIC_RELEASE);
	guard->unlock();
}

#ifdef __cplusplus
}
#endif

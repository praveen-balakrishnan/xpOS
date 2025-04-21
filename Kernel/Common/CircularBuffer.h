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

#ifndef XPOS_COMMON_CIRCULARBUFFER_H
#define XPOS_COMMON_CIRCULARBUFFER_H

#include "Common/String.h"

namespace Common
{

/**
 * A fixed size (in bytes) circular buffer that does not rely on dynamic memory allocation.
 */
template <int Size>
class FixedCircularBuffer
{
public:
    /**
     * Read up to count bytes from the buffer. Returns number of bytes read.
     */
    std::size_t read(std::size_t count, void* buf)
    {
        std::size_t readCount = 0;
        auto start = m_queueFront;

        while (!is_queue_empty() && count--) {
            m_queueFront += 1;
            m_queueFront %= Size;
            readCount++;
        }

        auto end = m_queueFront;

        if (start <= end) {
            memcpy(buf, m_queue + start, end - start);
        } else { 
            memcpy(buf, m_queue + start, Size - start);
            memcpy(static_cast<char*>(buf) + (Size - start), m_queue, end);
        }

        return readCount;
    }

    /**
     * Write up to count bytes from the buffer. Returns number of bytes written.
     */
    std::size_t write(std::size_t count, const void* buf)
    {
        std::size_t writeCount = 0;
        auto start = m_queueBack;

        while (!is_queue_full() && count--) {
            m_queueBack += 1;
            m_queueBack %= Size;
            writeCount++;
        }

        auto end = m_queueBack;

        if (start <= end) {
            memcpy(m_queue + start, buf, end - start);
        } else {
            memcpy(m_queue + start, buf, Size - start);
            memcpy(m_queue, static_cast<const char*>(buf) + (Size - start), end);
        }

        return writeCount;
    }

    bool is_queue_empty()
    {
        return m_queueFront == m_queueBack;
    }

    bool is_queue_full()
    {
        return (m_queueBack + 1) % Size == m_queueFront;
    }


private:
    int m_queueFront = 0;
    int m_queueBack = 0;
    char m_queue[Size];
};

}

#endif
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

#ifndef XPOS_COMMON_PRIORITYQUEUE_H
#define XPOS_COMMON_PRIORITYQUEUE_H

#include "Common/List.h"

namespace Common {
    /**
     * A data structure that provides constant time lookup of the largest element in the
     * queue.
     * 
     * @tparam T the type of element to be stored.
    */
    template<std::totally_ordered T>
    class PriorityQueue
    {
    public:
        using value_type = T;
        using reference = value_type&;
        using const_reference = const reference;

        PriorityQueue() = default;

        /**
         * Gets the element at the front of the priority queue (the largest element) in
         * constant time
         * 
         * @return reference to the front item.
        */
        const_reference top()
        {
            return *m_list.begin();
        }

        /**
         * Swaps the contents of two priority queues.
        */
        friend void swap(PriorityQueue& a, PriorityQueue& b)
        {
            std::swap(a.m_list, b.m_list);
        }

        /**
         * Removes the front (largest) element of the priority queue.
        */
        void pop()
        {
            auto it = m_list.begin();
            if (it == m_list.end())
                return;
            m_list.erase(it);
        }

        /**
         * Inserts an element into the priority queue. It is a linear time operation to
         * insert into the ordered queue.
        */
        void push(const T& element)
        {
            auto it = get_insert_iterator(element);
            m_list.insert(it, element);
        }

        /**
         * Inserts an element into the priority queue. It is a linear time operation to
         * insert into the ordered queue.
        */
        void push(T&& element)
        {
            auto it = get_insert_iterator(element);
            m_list.insert(it, std::move(element));
        }
        
        std::size_t size()
        {
            return m_list.size();
        }
    private:
        List<T> m_list;
        
        List<T>::Iterator get_insert_iterator(const T& element)
        {
            auto it = m_list.begin();
            for (;it != m_list.end(); ++it) {
                if (element > *it)
                    break;
            }
            return it;
        }
    };
}

#endif
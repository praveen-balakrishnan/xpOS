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

#ifndef XPOS_COMMON_DOUBLYLINKEDLIST_H
#define XPOS_COMMON_DOUBLYLINKEDLIST_H

#include <iterator>
#include <memory>

#include "Memory/KernelHeap.h"

namespace Common {

/**
 * This implementation of a doubly linked list is designed to be used by kernel
 * processes.
 * 
 * @tparam T type of element stored.
 */
template<typename T>
class List
{
private:
    template<bool IsConst>
    class IteratorBase;

    template<bool IsConst>
    friend class IteratorBase;
public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;
    using Iterator = IteratorBase<false>;
    using ConstIterator = IteratorBase<true>;

    /**
     * Constructs an empty doubly linked list.
    */
    List() = default;

    ~List()
    {
        clear();
    }

    List(List const& rhs)
        : List()
    {
        for (auto const& item : rhs)
            push_back(item);
    }

    List& operator=(List rhs)
    {
        swap(rhs, *this);
        return *this;
    }

    List(List&& rhs)
        : List()
    {
        swap(rhs, *this);
    }

    /**
     * Initialises a doubly linked list with a list of initial elements.
     * 
     * @param init the initial elements of the list.
     * @example Common::List<int>({1, 2, 3})
    */
    List(std::initializer_list<value_type> init)
    {
        for (auto& element : init)
            push_back(element);
    }

    /**
     * Swaps the data of two doubly linked lists.
    */
    friend void swap(List& a, List& b)
    {
        using std::swap;
        swap(a.m_head, b.m_head);
        swap(a.m_tail, b.m_tail);
        swap(a.m_size, b.m_size);
    }

    std::size_t size() const
    {
        return m_size;
    }

    /**
     * Empties the doubly linked list.
    */
    void clear()
    {
        for (auto* node = m_head; node;)
        {
            auto* next = node->next;
            delete node;
            node = next;
        }
        m_head = nullptr;
        m_tail = nullptr;
        m_size = 0;
    }

    /**
     * Inserts an element into the doubly linked list.
     * 
     * @param it the iterator to insert the element before.
     * @param value the element to copy into the list.
     * @return an iterator pointing to the inserted element.
    */
    Iterator insert(Iterator it, const value_type& value)
    {
        auto* node = new ListNode(value);
        return insert(it, node);
    }

    /**
     * Inserts an element into the doubly linked list.
     * 
     * @param it the iterator to insert the element before.
     * @param value the element to move into the list.
     * @return an iterator pointing to the inserted element.
    */
    Iterator insert(Iterator it, value_type&& value)
    {
        auto* node = new ListNode(std::move(value));
        return insert(it, node);
    }

    /**
     * Constructs an element in-place at the location in the list.
     * 
     * @param it the iterator to insert the element before.
     * @param args the arguments forwarded to the element's constructor.
     * @return an iterator pointing to the inserted element.
    */
    template<class... Args>
    Iterator emplace(Iterator it, Args&&... args)
    {
        auto* node = new ListNode(std::forward<Args>(args)...);
        return insert(it, node);
    }

    /**
     * Inserts an element at the end of the list.
    */
    void push_back(const value_type& value)
    {
        auto* node = new ListNode(value);
        insert(end(), node);
    }

    /**
     * Inserts an element at the end of the list.
    */
    void push_back(value_type&& value)
    {
        auto* node = new ListNode(std::move(value));
        insert(end(), node);
    }

    /**
     * Constructs an element in-place at the end in the list.
     * Equivalent to
     * @code
     * *emplace(end(), std::forward<Args>(args))
     * @endcode
    */
    template<class... Args>
    reference emplace_back(Args&&... args)
    {
        auto* node = new ListNode(std::forward<Args>(args)...);
        insert(end(), node);
        return node->value;
    }

    /**
     * Erases the element pointed to by the iterator.
     * 
     * @param it the iterator pointing to the element to be erased.
     * @return an iterator following the element erased.
    */
    Iterator erase(Iterator it)
    {
        m_size--;
        auto* node = it.m_node;
        auto ret = Iterator(node->next, this);
        // Remove the node and change the node before and after to point to each other.
        if (node->prev) {
            node->prev->next = node->next;
        } else {
            m_head = node->next;
        }

        if (node->next) {
            node->next->prev = node->prev;
        } else {
            m_tail = node->prev;
        }

        delete node;
        return ret;
    }

    reference front()
    {
        return m_head->value;
    }

    const reference front() const
    {
        return m_head->value;
    }

    reference back()
    {
        return m_tail->value;
    }

    const reference back() const
    {
        return m_tail->value;
    }

    Iterator begin()
    {
        return Iterator(m_head, this);
    }

    Iterator end()
    {
        return Iterator(nullptr, this);
    }

    ConstIterator begin() const
    {
        return cbegin();
    }

    ConstIterator end() const
    {
        return cend();
    }

    ConstIterator cbegin() const
    {
        return ConstIterator(m_head, this);
    }

    ConstIterator cend() const
    {
        return ConstIterator(nullptr, this);
    }

private:
    struct ListNode;

    template<bool IsConst>
    class IteratorBase
    {
        friend class List;
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<IsConst, const value_type*, value_type*>;
        using reference = std::conditional_t<IsConst, const value_type&, value_type&>;
        using iterator_category = std::bidirectional_iterator_tag;

        IteratorBase() = default;

        /**
         * This template is only suitable if constructing a const iterator from
         * any iterator or a non const iterator from another non const
         * iterator.
        */
        template<bool WasConst, class = std::enable_if_t<IsConst || !WasConst>>
        IteratorBase(const IteratorBase<WasConst>& rhs)
            : m_node(rhs.m_node)
            , m_list(rhs.m_list)
        {
            /**
            * We want to be able to construct a const iterator from a non-const
            * iterator, but not vice versa. As a result, this template is only
            * suitable if we are constructing a const iterator (IsConst), or we 
            * are constructing from a non const iterator (not WasConst).
            */
        }

        template<bool WasConst, class = std::enable_if_t<IsConst || !WasConst>>
        IteratorBase(IteratorBase<WasConst>&& rhs)
            : m_node(rhs.m_node)
            , m_list(rhs.m_list)
        {
            /**
            * We want to be able to construct a const iterator from a non-const
            * iterator, but not vice versa. As a result, this template is only
            * suitable if we are constructing a const iterator (IsConst), or we 
            * are constructing from a non const iterator (not WasConst).
            */
        }

        template<bool WasConst, class = std::enable_if_t<IsConst || !WasConst>>
        IteratorBase& operator=(const IteratorBase<WasConst>& rhs)
        {
            m_node = rhs.m_node;
            m_list = rhs.m_list;
            return *this;
        }

        template<bool WasConst, class = std::enable_if_t<IsConst || !WasConst>>
        IteratorBase& operator=(IteratorBase<WasConst>&& rhs)
        {
            m_node = rhs.m_node;
            m_list = rhs.m_list;
            return *this;
        }

        IteratorBase& operator++()
        {
            m_node = m_node->next;
            return *this;
        }

        IteratorBase operator++(int)
        {
            auto copy = *this;
            m_node = m_node->next;
            return copy;
        }

        IteratorBase& operator--()
        {
            if (m_node == nullptr)
                m_node = m_list->m_tail;
            else
                m_node = m_node->prev;
            return *this;
        }

        IteratorBase operator--(int)
        {
            auto copy = *this;
            if (m_node == nullptr)
                m_node = m_list->m_tail;
            else
                m_node = m_node->prev;
            return copy;
        }

        value_type& operator*() const
        {
            return m_node->value;
        }

        value_type* operator->() const
        {
            return &m_node->value;
        }

        friend bool operator==(const IteratorBase& lhs, const IteratorBase& rhs)
        {
            return lhs.m_node == rhs.m_node;
        }
        
    private:
        ListNode* m_node { nullptr };
        const List* m_list { nullptr };
        IteratorBase(ListNode* node, const List* list)
            : m_node(node)
            , m_list(list)
        {}
    };

    struct ListNode
    {
    public:
        ListNode* next = nullptr;
        ListNode* prev = nullptr;
        value_type value;
        /*ListNode(const T& value)
            : value(value)
        {}

        ListNode(T&& value)
            : value(std::move(value))
        {}*/

        template<typename... Args>
        explicit ListNode(Args&&... args)
            : value(std::forward<Args>(args)...)
        {}
    };

    ListNode* m_head = nullptr;
    ListNode* m_tail = nullptr;
    uint64_t m_size = 0;

    Iterator insert(Iterator it, ListNode* node)
    {
        m_size++;
        if (!m_head) {
            m_head = node;
            m_tail = node;
        } else if (it == end()) {
            m_tail->next = node;
            node->prev = m_tail;
            m_tail = node;
        } else {
            node->next = it.m_node;
            node->prev = it.m_node->prev;
            if (it.m_node->prev)
                it.m_node->prev->next = node;
            else
                m_head = node;
            it.m_node->prev = node;
        }
        return Iterator(node, this);
    }

    static_assert(std::bidirectional_iterator<Iterator>);
};

}

#endif
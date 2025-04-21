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

#ifndef XPOS_COMMON_VECTOR_H
#define XPOS_COMMON_VECTOR_H

#include <cstdint>
#include <iterator>

namespace Common
{

/**
 * This implementation of a vector is designed to be used by kernel
 * processes.
 * 
 * @tparam T type of element stored.
 */
template<typename T>
class Vector
{
private:
    template<bool IsConst>
    class IteratorBase;

public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;

    using Iterator = IteratorBase<false>;
    using ConstIterator = IteratorBase<true>;

    Vector() {}

    ~Vector()
    {
        if (size())
            delete[] m_start;
    }

    /**
     * Initialise with enough space reserved for a given number of elements.
     */
    explicit Vector(size_type count)
    {
        reserve(count);
    }

    /**
     * Initialises a vector filled with an element.
     * 
     * @param count the number of elements to populate.
     * @param value the repeated element that populates the vector.
     */
    Vector(size_type count, const T& value)
    {
        reserve(count);

        for (size_type i = 0; i < count; i++) {
            new (m_start + count) T(value);
        }
    }

    Vector(const Vector& vector)
    {
        reserve(vector.capacity());
        for (auto& item : vector)
            push_back(item);
    }

    /**
     * Swaps the data of two vectors.
     */
    friend void swap(Vector& a, Vector& b)
    {
        using std::swap;
        swap(a.m_start, b.m_start);
        swap(a.m_finish, b.m_finish);
        swap(a.m_capacityEnd, b.m_capacityEnd);
    }

    Vector& operator=(Vector v)
    {
        swap(v, *this);
        return *this;
    }

    Vector(Vector&& vector)
        : m_start(vector.m_start)
        , m_finish(vector.m_finish)
        , m_capacityEnd(vector.m_capacityEnd)
    {
        vector.m_start = nullptr;
        vector.m_finish = nullptr;
        vector.m_capacityEnd = nullptr;
    }

    /**
     * Reserve enough space for a given number of elements.
     */
    void reserve(size_type new_cap)
    {
        if (new_cap <= capacity())
            return;
        

        auto* newCharAlloc = std::launder(new uint8_t[sizeof(value_type) * new_cap]);
        auto* newAlloc = reinterpret_cast<pointer>(newCharAlloc);

        auto sz = size();

        if (sz) {
            for (std::size_t i = 0; i < sz; i++) {
                newAlloc[i] = m_start[i];
            }
            delete[] m_start;
        }
        
        m_start = newAlloc;
        m_finish = m_start + sz;
        m_capacityEnd = m_start + new_cap;
    }

    /**
     * Returns the number of elements that can be stored without requiring a reallocation.
     */
    size_type capacity() const
    {
        return static_cast<size_type>(m_capacityEnd - m_start);
    }

    size_type size() const
    {
        return static_cast<size_type>(m_finish - m_start);
    }

    reference operator[](size_type position)
    {
        return *(m_start + position);
    }

    const_reference operator[](size_type position) const
    {
        return *(m_start + position);
    }

    Iterator begin()
    {
        return Iterator(m_start);
    }

    ConstIterator begin() const
    {
        return ConstIterator(m_start);
    }

    ConstIterator cbegin() const
    {
        return ConstIterator(m_start);
    }

    Iterator end()
    {
        return Iterator(m_finish);
    }

    ConstIterator end() const
    {
        return ConstIterator(m_finish);
    }

    ConstIterator cend() const
    {
        return ConstIterator(m_finish);
    }

    reference front()
    {
        return *m_start;
    }

    const_reference front() const
    {
        return *m_start;
    }

    reference back()
    {
        return *m_finish;
    }

    const_reference back() const
    {
        return *m_finish;
    }

    T* data()
    {
        return m_start;
    }

    const T* data() const
    {
        return m_start;
    }

    void clear()
    {
        auto init_size = size();
        for (std::size_t i = 0; i < init_size; i++) {
            pop_back();
        }
    }

    void push_back(const T& value)
    {
        if (size() >= capacity())
            reserve(capacity() * 2 + 1);

        new (m_finish) T(value);
        m_finish++;
    }

    void push_back(T&& value)
    {
        if (size() >= capacity())
            reserve(capacity() * 2 + 1);

        new (m_finish) T(std::move(value));
        m_finish++;
    }

    void pop_back()
    {
        (&back())->~T();
        m_finish--;
    }

private:
    pointer m_start = nullptr;
    pointer m_finish = nullptr;
    pointer m_capacityEnd = nullptr;

    template<bool IsConst>
    class IteratorBase
    {
        friend class Vector;
    public:
        using value_type = T;
        using element_type = std::conditional_t<IsConst, const value_type, value_type>;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<IsConst, const value_type*, value_type*>;
        using reference = std::conditional_t<IsConst, const value_type&, value_type&>;
        using iterator_category = std::contiguous_iterator_tag;

        IteratorBase()
            : m_item(nullptr)
        {}

        /**
         * This template is only suitable if constructing a const iterator from
         * any iterator or a non const iterator from another non const
         * iterator.
        */
        template<bool WasConst, class = std::enable_if_t<IsConst || !WasConst>>
        IteratorBase(const IteratorBase<WasConst>& rhs)
            : m_item(rhs.m_item)
        {
            /**
            * We want to be able to construct a const iterator from a non-const
            * iterator, but not vice versa. As a result, this template is only
            * suitable if we are constructing a const iterator (IsConst), or we 
            * are constructing from a non const iterator (not WasConst).
            */
        }

        reference operator*() const
        {
            return *m_item;
        }

        pointer operator->() const
        {
            return m_item;
        }

        reference operator[](int i) const
        {
            return m_item[i];
        }

        IteratorBase& operator++()
        {
            m_item++;
            return *this;
        }

        IteratorBase& operator--()
        {
            m_item--;
            return *this;
        }

        IteratorBase operator++(int)
        {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        IteratorBase operator--(int)
        {
            auto copy = *this;
            --(*this);
            return copy;
        }

        IteratorBase& operator+=(int i)
        {
            m_item += i;
            return *this;
        }

        IteratorBase& operator-=(int i)
        {
            m_item -= i;
            return *this;
        }

        friend int operator-(IteratorBase lhs, IteratorBase rhs) 
        {
            return reinterpret_cast<int>(lhs.m_item - rhs.m_item);
        }

        friend IteratorBase operator+(IteratorBase lhs, int val)
        {
            return IteratorBase(lhs.m_item + val);
        }

        friend IteratorBase operator+(int val, IteratorBase rhs)
        {
            return IteratorBase(val + rhs.m_item);
        }

        friend IteratorBase operator-(IteratorBase lhs, int val)
        {
            return IteratorBase(lhs.m_item - val);
        }

        friend auto operator<=>(IteratorBase, IteratorBase) = default;
        
    private:

        IteratorBase(pointer item)
            : m_item(item)
        {}

        pointer m_item;
    };

    ///FIXME: this assert is failing, but should pass!
    //static_assert(std::contiguous_iterator<Iterator>);
};

}

#endif
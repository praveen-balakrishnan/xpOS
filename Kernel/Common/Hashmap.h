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

#include <utility>

#include "Common/List.h"
#include "Common/Hash/MurmurHash3.h"

#include "panic.h"
#ifndef XPOS_COMMON_HASHMAP_H
#define XPOS_COMMON_HASHMAP_H

namespace Common {

/**
 * A data structure that allows amortised constant time lookup for key-value
 * pairs.
 * 
 * @tparam K the key type.
 * @tparam V the value type.
*/
template<typename K, typename V>
class Hashmap
{
private:
    template<bool IsConst>
    class IteratorBase;
public:
    using value_type = std::pair<K, V>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    using Iterator = IteratorBase<false>;
    using ConstIterator = IteratorBase<true>;
private:
    using Bucket = List<value_type>;
public:

    Hashmap()
        : m_bucketArray(new Bucket[m_bucketCount])
    {
    }

    ~Hashmap()
    {
        if (m_bucketArray) {
            clear();
            delete[] m_bucketArray;
        }
    }

    Hashmap(const Hashmap& other)
        : Hashmap()
    {
        for (auto& item : other) {
            insert(item);
        }
    }

    Hashmap& operator=(Hashmap other)
    {
        swap(*this, other);
        return *this;
    }

    Hashmap(Hashmap&& other)
        : m_bucketCount(other.m_bucketCount)
        , m_size(other.m_size)
        , m_bucketArray(other.m_bucketArray)
    {
        other.m_bucketCount = 0;
        other.m_size = 0;
        other.m_bucketArray = nullptr;
    }

    /**
     * Swaps the contents of two Hashmaps.
     * 
     * @param a a Hashmap to swap.
     * @param b the other Hashmap to swap with.
    */
    friend void swap(Hashmap& a, Hashmap& b)
    {
        using std::swap;
        swap(a.m_bucketCount, b.m_bucketCount);
        swap(a.m_size, b.m_size);
        swap(a.m_bucketArray, b.m_bucketArray);
    }

    void clear()
    {
        for (std::size_t bucket = 0; bucket < m_bucketCount; bucket++)
            m_bucketArray[bucket].clear();
        m_size = 0;
    }

    std::size_t size() const
    {
        return m_size;
    }

    /**
     * Inserts a key-value pair. This invalidates iterators.
     * 
     * @param value the pair to copy into the Hashmap.
     * @return a pair where the first element is an iterator to the new pair inserted
     * or the pair that blocked insertion, and the second element is true if the pair
     * was inserted.
    */
    std::pair<Iterator, bool> insert(const value_type& value)
    {
        auto it = find(value.first);
        if (it != end()) {
            return std::pair(it, false);
        }
        return std::pair(force_insert(value), true);
    }

    /**
     * Inserts a key-value pair. This invalidates iterators.
     * 
     * @param value the pair to move into the Hashmap.
     * @return a pair where the first element is an iterator to the new pair inserted
     * or the pair that blocked insertion, and the second element is true if the pair
     * was inserted.
    */
    std::pair<Iterator, bool> insert(value_type&& value)
    {
        auto it = find(value.first);
        if (it != end()) {
            return std::pair(it, false);
        }
        return std::pair(force_insert(std::move(value)), true);
    }
    
    /**
     * Inserts a key-value pair. This invalidates iterators.
     * 
     * @param value the constructor arguments of the pair.
     * @return a pair where the first element is an iterator to the new pair inserted
     * or the pair that blocked insertion, and the second element is true if the pair
     * was inserted.
    */
    template<class P, std::enable_if_t<std::is_constructible<value_type, P&&>::value, bool> = true>
    std::pair<Iterator, bool> insert(P&& value)
    {
        auto constructedValue = value_type(std::forward<P>(value));
        auto it = find(constructedValue.first);
        if (it != end()) {
            return std::pair(it, false);
        }
        return std::pair(force_insert(std::move(constructedValue)), true);
    }

    /**
     * Constructs the key-value pair in-place.
    */
    template<class... Args>
    std::pair<Iterator, bool> emplace(Args&&... args)
    {
        return insert(value_type(args...));
    }

    Iterator begin()
    {
        if (size() == 0)
            return end();
        return Iterator(m_bucketArray->begin(), m_bucketArray, m_bucketArray + m_bucketCount);
    }

    Iterator end()
    {
        return Iterator();
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
        if (size() == 0)
            return cend();
        return ConstIterator(m_bucketArray->begin(), m_bucketArray, m_bucketArray + m_bucketCount);
    }

    ConstIterator cend() const
    {
        return ConstIterator();
    }

    /**
     * Finds the key-value pair with the given key in amortised constant time.
     * 
     * @return an iterator to the pair if one exists, or end() if one does not exist.
    */
    Iterator find(const K& key)
    {
        // Search through the bucket to find the key.
        auto* bucket = m_bucketArray + bucket_index(key);
        for (auto it = bucket->begin(); it != bucket->end(); ++it) {
            if (it->first == key) 
                return Iterator(it, bucket, m_bucketArray + m_bucketCount);
        }
        return Iterator();
    }

    /**
     * Finds the key-value pair with the given key in amortised constant time.
     * 
     * @return an iterator to the pair if one exists, or end() if one does not exist.
    */
    ConstIterator find(const K& key) const
    {
        auto* bucket = m_bucketArray + bucket_index(key);
        for (auto it = bucket->begin(); it != bucket->end(); ++it) {
            if (it->first == key)
                return ConstIterator(it, bucket, m_bucketArray + m_bucketCount);
        }
        return ConstIterator();
    }

    /**
     * Accesses the value associated with the given key in amortised constant time.
     * 
     * @param key the key to lookup for the associated value.
     * @return a reference to the value.
    */
    V& operator[](const K& key)
    {
        auto it = find(key);
        KERNEL_ASSERT(it != end());
        return it->second;
    }

    /**
     * Accesses the value associated with the given key in amortised constant time.
     * 
     * @param key the key to lookup for the associated value.
     * @return a reference to the value.
    */
    V& operator[](K&& key)
    {
        auto it = find(key);
        KERNEL_ASSERT(it != end());
        return it->second;
    }

    /**
     * Remove the key-value pair associated with the given key and rebalance if needed.
     * This invalidates iterators.
     * 
     * @param key the key of the pair to be erased.
    */
    void erase(const K& key)
    {
        auto it = find(key);
        if (it != end()) {
            it.m_bucket->erase(it.m_bucketIterator);
            update_size_on_removal();
        }
    }

    /**
     * Remove the key-value pair pointed to by the iterator.
     * This invalidates iterators.
     * 
     * @param it an iterator pointing to the pair to erase.
    */
    void erase(Iterator it)
    {
        if (it != end()) {
            it.m_bucket->erase(it.m_bucketIterator);
            update_size_on_removal();
        }
    }

private:

    template<bool IsConst>
    class IteratorBase
    {
        friend class Hashmap;
    public:
        using value_type = std::pair<K, V>;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<IsConst, const value_type*, value_type*>;
        using reference = std::conditional_t<IsConst, const value_type&, value_type&>;
        using iterator_category = std::forward_iterator_tag;

        IteratorBase() = default;

        template<bool WasConst, class = std::enable_if_t<IsConst || !WasConst>>
        IteratorBase(const IteratorBase<WasConst>& rhs)
            : m_bucketIterator(rhs.m_bucketIterator)
            , m_bucket(rhs.m_bucket)
            , m_endBucket(rhs.m_endBucket)
        {
            /**
            * We want to be able to construct a const iterator from a non-const
            * iterator, but not vice versa. As a result, this template is only
            * suitable if we are constructing a const iterator (IsConst), or we 
            * are constructing from a non const iterator (not WasConst).
            */
        }
        
        IteratorBase& operator++()
        {
            if (!m_bucket)
                return *this;
            
            /*do {
                m_bucketIterator++;
                if (m_bucketIterator == m_bucket->end()) {
                    m_bucket++;
                    if (m_bucket == m_endBucket) {
                        m_bucket = nullptr;
                        return *this;
                    }
                    m_bucketIterator = m_bucket->begin();
                }
            } while (m_bucketIterator == m_bucket->end());*/

            ++m_bucketIterator;
            while (m_bucketIterator == m_bucket->end()) {
                ++m_bucket;
                if (m_bucket == m_endBucket) {
                    m_bucket = nullptr;
                    return *this;
                }
                m_bucketIterator = m_bucket->begin();
            }
            
            return *this;
        }

        IteratorBase operator++(int)
        {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        reference operator*() const
        {
            return *m_bucketIterator;
        }

        pointer operator->() const
        {
            return &*m_bucketIterator;
        }

        friend bool operator==(const IteratorBase& lhs, const IteratorBase& rhs)
        {
            if (lhs.m_bucket == rhs.m_bucket) {
                return (lhs.m_bucket == nullptr) || (lhs.m_bucketIterator == rhs.m_bucketIterator);
            }
            return false;
        }

        friend bool operator!=(const IteratorBase& lhs, const IteratorBase& rhs)
        {
            return !(lhs == rhs);
        }

    private:

        IteratorBase(List<value_type>::Iterator bucketIterator, Bucket* bucket, Bucket* endBucket)
            : m_bucketIterator(bucketIterator)
            , m_bucket(bucket)
            , m_endBucket(endBucket)
        {}
public:
        Bucket::Iterator m_bucketIterator;
        Bucket* m_bucket = nullptr;
        Bucket* m_endBucket = nullptr;
    };
    std::size_t m_bucketCount = 1;
    std::size_t m_size = 0;
    Bucket* m_bucketArray = nullptr;
    static constexpr float MAX_LOAD_FACTOR = 1;

    std::size_t bucket_index(const K& key)
    {
        uint64_t hashedIntegers[2];
        // This is an open source hash.
        MurmurHash3_x64_128(&key, sizeof(K), 3, hashedIntegers);
        return hashedIntegers[0] % m_bucketCount;
    }

    Iterator force_insert(const value_type& value)
    {
        update_size_on_insertion();
        auto bucketIndex = bucket_index(value.first);
        m_bucketArray[bucketIndex].push_back(value);
        auto listIt = --m_bucketArray[bucketIndex].end();
        return Iterator(listIt, m_bucketArray + bucketIndex, m_bucketArray + m_bucketCount);
    }

    Iterator force_insert(value_type&& value)
    {
        update_size_on_insertion();
        auto bucketIndex = bucket_index(value.first);
        m_bucketArray[bucketIndex].push_back(std::move(value));
        auto listIt = --m_bucketArray[bucketIndex].end();
        return Iterator(listIt, m_bucketArray + bucketIndex, m_bucketArray + m_bucketCount);
    }

    void update_size_on_insertion()
    {
        m_size++;
        // If we have a high load factor (avg. bucket size), then increase the number of buckets.
        if (m_size <= 2 * m_bucketCount)
            return;
        
        /*auto oldBucketCount = m_bucketCount;
        m_bucketCount *= 2;
        auto newBuckets = new Bucket[m_bucketCount];
        // Rehash each element and populate the new buckets.
        for (std::size_t i = 0; i < oldBucketCount; i++) {
            for (auto it = m_bucketArray[i].begin(); it != m_bucketArray[i].end(); ++it)
                newBuckets[bucket_index(it->first)].push_back(*it);
        }
        
        delete[] m_bucketArray;
        m_bucketArray = newBuckets;*/
        
    }

    void update_size_on_removal()
    {
        m_size--;
        // Check if the number of buckets can be shrunk.
        if (m_size != 0 && m_bucketCount <= 3 * m_size )
            return;
        
        /*auto oldBucketCount = m_bucketCount;
        m_bucketCount /= 2;
        if (m_bucketCount <= 0)
            m_bucketCount = 1;
        auto newBuckets = new Bucket[m_bucketCount]();
        for (std::size_t i = 0; i < oldBucketCount; i++) {
            for (auto it = m_bucketArray[i].begin(); it != m_bucketArray[i].end(); ++it)
                newBuckets[bucket_index(it->first)].push_back(*it);
        }
        delete[] m_bucketArray;
        m_bucketArray = newBuckets;*/
    }

    static_assert(std::forward_iterator<Iterator>);
};

} // namespace
#endif
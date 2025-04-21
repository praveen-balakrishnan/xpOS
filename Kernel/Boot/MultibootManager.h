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

#ifndef XPOS_MULTIBOOTMANAGER_H
#define XPOS_MULTIBOOTMANAGER_H
#include <cstdint>
#include "Boot/multiboot2.h"
#include "Memory/Memory.h"
#include <iterator>

namespace Multiboot
{

    struct [[gnu::packed]] multiboot_info
    {
        multiboot_uint32_t total_size;
        multiboot_uint32_t reserved;

        multiboot_tag* tags_start()
        {
            return reinterpret_cast<multiboot_tag*>(this + 1);
        }
        //multiboot_tag tags[];
    };

    multiboot_info* get_structure();
    void set_structure(multiboot_info* structure);

    
    class MultibootTagCollection
    {
    private:
        template<bool IsConst>
        class IteratorBase;

    public:
        MultibootTagCollection(uint32_t tagType) : m_desiredTag(tagType)
        {}

        using Iterator = IteratorBase<false>;
        using ConstIterator = IteratorBase<true>;

        Iterator begin()
        {
            return Iterator(get_structure()->tags_start(), m_desiredTag);
        }

        Iterator end()
        {
            return Iterator();
        }

        ConstIterator begin() const
        {
            return cbegin();
        }

        ConstIterator cbegin() const
        {
            return ConstIterator(get_structure()->tags_start(), m_desiredTag);
        }

        ConstIterator end() const
        {
            return cend();
        }

        ConstIterator cend() const
        {
            return ConstIterator();
        }

    private:
        template<bool IsConst>
        class IteratorBase
        {
            friend class MultibootManager;
        public:
            using value_type = multiboot_tag;
            using difference_type = std::ptrdiff_t;
            using pointer = std::conditional_t<IsConst, const value_type*, value_type*>;
            using reference = std::conditional_t<IsConst, const value_type&, value_type&>;
            using iterator_category = std::forward_iterator_tag;

            IteratorBase() : m_tag(nullptr), m_desiredTag(0)
            {}

            IteratorBase(multiboot_tag* tag, uint32_t tagType) : m_tag(tag), m_desiredTag(tagType)
            {
                if (!m_tag)
                    return;
                if (m_tag->type != m_desiredTag)
                    this->advance();
            }

            IteratorBase& operator++()
            {
                advance();
                return *this;
            }

            IteratorBase operator++(int)
            {
                auto copy = *this;
                this->advance();
                return copy;
            }

            value_type& operator*() const
            {
                return *m_tag;
            }

            value_type* operator->() const
            {
                return m_tag;
            }

            friend bool operator==(const IteratorBase& lhs, const IteratorBase& rhs)
            {
                return lhs.m_tag == rhs.m_tag;
            }

        private:
            void advance()
            {
                do
                {
                    // Multiboot2 tags are padded out so that each consecutive tag starts on a 8-byte boundary.
                    m_tag = reinterpret_cast<multiboot_tag*>(reinterpret_cast<uint8_t*>(m_tag) + BYTE_ALIGN_UP(m_tag->size, 8));
                }
                while (m_tag->type != m_desiredTag && m_tag->type != MULTIBOOT_TAG_TYPE_END);

                if (m_tag->type == MULTIBOOT_TAG_TYPE_END)
                    m_tag = nullptr;
            }
            value_type* m_tag;
            uint32_t m_desiredTag;
        };
        uint32_t m_desiredTag;
    };

}

#endif

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

#ifndef XPOS_COMMON_OPTIONAL_H
#define XPOS_COMMON_OPTIONAL_H

#include <new>
#include <utility>
#include "panic.h"

namespace Common
{
    struct __NulloptType
    {
        enum class __NulloptEnum { __None };
        constexpr __NulloptType(__NulloptEnum) noexcept { }
    };

    static inline constexpr __NulloptType Nullopt { __NulloptType::__NulloptEnum::__None };

    /**
     * A wrapper to manage an optionally contained object.
     * 
     * @tparam T the type of object to be stored.
    */
    template <typename T>
    class Optional
    {
    public:
        /**
         * Construct an Optional that does not contain a value.
        */
        Optional() = default;
        Optional(__NulloptType) {}

        /**
         * Construct an Optional holding a value.
         * 
         * @param value the value to be copied into the Optional.
        */
        Optional(const T& value)
            : m_hasValue(true)
        {
            new (m_data) T(value);
        }

        /**
         * Construct an Optional holding a value.
         * 
         * @param value the value to be moved into the Optional.
        */
        Optional(T&& value)
            : m_hasValue(true)
        {
            new (m_data) T(std::move(value));
        }

        /**
         * Construct an Optional that constructs a value.
         * 
         * @param args varadic arguments forwarded to construct the value.
        */
        template <typename... Args>
        Optional(std::in_place_t, Args&&... args)
        {
            emplace(std::forward<Args>(args)...);
        }

        Optional(const Optional& other)
            : m_hasValue(other.m_hasValue)
        {
            if (other.m_hasValue)
            {
                new (m_data) T(other.value());
            }
        }

        Optional(Optional&& other)
            : m_hasValue(other.m_hasValue)
        {
            if (other.m_hasValue)
            {
                new (m_data) T(std::move(other.value()));
            }
        }

        Optional& operator=(const Optional& other)
        {
            Optional temp {other};
            swap(*this, temp);
            return *this;
        }

        Optional& operator=(Optional&& other)
        {
            Optional temp {std::move(other)};
            swap(*this, temp);
            return *this;
        }
        
        /**
         * Resets the Optional to not contain a value.
        */
        void reset()
        {
            if (m_hasValue)
            {
                auto* value = reinterpret_cast<T*>(m_data);
                value->~T();
                m_hasValue = false;
            }
        }

        /**
         * Constructs a value in-place in the Optional.
         * 
         * @param args varadic arguments forwarded to the constructor of the value.
        */
        template<typename... Args>
        void emplace(Args&&... args)
        {
            reset();
            new (m_data) T(std::forward<Args>(args)...);
            m_hasValue = true;
        }

        /**
         * Swaps the contents of two Optionals.
        */
        friend void swap(Optional& a, Optional& b)
        {
            using std::swap;

            if (a.has_value() && b.has_value()) {
                swap(*a.pointer(), *b.pointer());
            } else if (b.has_value()) {
                new (a.m_data) T(std::move(b.value()));
                a.m_hasValue = true;
                b.reset();
            } else if (a.has_value()) {
                new (b.m_data) T(std::move(a.value()));
                b.m_hasValue = true;
                a.reset();
            }
        }

        ~Optional()
        {
            reset();
        }

        /**
         * Returns the value of the Optional if it exists, or the passed default_value 
         * if it does not.
         * default_value is a forwarding reference that must be convertible to the type of
         * value stored.
         * 
         * @param default_value a forwarding reference to an alternative value.
         * @return the contained value if it exists, or the passed argument if it does not.
        */
        template<typename U>
        constexpr T value_or(U&& default_value) const&
        {
            return has_value() ? **this : static_cast<T>(std::forward<U>(default_value));
        }

        /**
         * Returns the value of the Optional if it exists, or the passed default_value if it does not.
         * default_value is a forwarding reference that must be convertible to the type of value stored.
         * 
         * @param default_value a forwarding reference to an alternative value.
         * @return the contained value if it exists, or the passed argument if it does not.
        */
        template<typename U>
        constexpr T value_or(U&& default_value) &&
        {
            return has_value() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
        }

        /**
         * Returns if the Optional object contains a value.
         * 
         * @return whether a value is contained.
        */
        bool has_value() const
        {
            return m_hasValue;
        }

        explicit operator bool() const
        {
            return has_value();
        }

        bool operator==(const Optional& other) const
        {
            return *this == other;
        }

        T& value()
        {
            return *pointer();
        }

        const T& value() const
        {
            return *pointer();
        }
        
        T& operator*()
        {
            return value();
        }

        const T& operator*() const
        {
            return value();
        }

        T* operator->() 
        {
            return pointer();
        }

        const T* operator->() const
        {
            return pointer();
        }

    private:
        alignas(T) uint8_t m_data[sizeof(T)];
        bool m_hasValue = false;

        const T* pointer() const
        {
            KERNEL_ASSERT(has_value());
            return std::launder(reinterpret_cast<const T*>(m_data));
        }

        T* pointer()
        {
            KERNEL_ASSERT(has_value());
            return std::launder(reinterpret_cast<T*>(m_data));
        }
    };
}

#endif
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

#ifndef XPOS_COMMON_EXPECTED_H
#define XPOS_COMMON_EXPECTED_H

#include <utility>

namespace Common
{
    template<typename E>
    class Unexpected
    {
    public:
        Unexpected(E&& error)
            : m_error(std::move(error)) {}
        
        template<typename... Args>
        Unexpected(Args&&... args) : m_error(std::forward<Args>(args)...) {}
    private:
        E m_error;
    };

    /**
     * A wrapper to store either of two values: an expected value or an unexpected value.
     * It always contains one of the two values.
     * 
     * @tparam T the expected value type.
     * @tparam E the unexpected value type.
    */
    template<typename T, typename E>
    class Expected
    {
    public:
        /**
         * Default constructs the expected value.
        */
        Expected()
            : m_hasValue(true)
        {
            new (m_valueUnion.value) T();
        }

        /**
         * Constructs an Expected holding a value.
         * 
         * @param value the value to be copied into the Expected.
         */
        Expected(const T& value)
            : m_hasValue(true)
        {
            new (m_valueUnion.value) T(value);
        }

        /**
         * Constructs an Expected holding a value.
         * 
         * @param value the value to be moved into the Expected.
         */
        Expected(T&& value)
            : m_hasValue(true)
        {
            new (m_valueUnion.value) T(std::move(value));
        }

        /**
         * Constructs the expected value in-place with arguments.
         * 
         * @param args varadic arguments forwarded to construct the expected value.
        */
        template <typename... Args>
        Expected(std::in_place_t, Args&&... args)
        {
            emplace(forward<Args>(args)...);
        }

        /**
         * Constructs the unexpected value with an Unexpected object.
         * The type parameter E must match.
         * 
         * @param error the Unexpected object containing the error.
        */
        Expected(const Unexpected<E>& error)
        {
            new (m_valueUnion.error) Unexpected<E>(error);
        }

        /**
         * Constructs the unexpected value with an Unexpected object.
         * The type parameter E must match.
         * 
         * @param error the Unexpected object containing the error.
        */
        Expected(Unexpected<E>&& error)
        {
            new (m_valueUnion.error) Unexpected<E>(std::move(error));
        }

        Expected(const Expected& other)
            : m_hasValue(other.m_hasValue)
        {
            if (other.m_hasValue)
            {
                new (m_valueUnion.value) T(other.value());
            }
            else
            {
                new (m_valueUnion.error) Unexpected(other.value());
            }
        }

        Expected& operator=(const Expected& other)
        {
            Expected temp {other};
            temp.swap(*this);
            return *this;
        }

        Expected& operator=(Expected&& other)
        {
            Expected temp {std::move(other)};
            temp.swap(*this);
            return *this;
        }

        ~Expected()
        {
            reset();
        }

        /**
         * Destroys the contained value and constructs the expected value in-place.
         * 
         * @param args varadic arguments forwarded to construct the expected value.
        */
        template<typename... Args>
        void emplace(Args&&... args)
        {
            reset();
            new (m_valueUnion.value) T(std::forward<Args>(args)...);
            m_hasValue = true;
        }

        /**
         * Swaps the contents of two Expecteds.
        */
        friend void swap(Expected& a, Expected& b)
        {
            if (a.has_value() && b.has_value())
            {
                std::swap(*a.value_pointer(), *b.value_pointer());
            }
            else if (b.has_value())
            {
                auto tmp = std::move(b.value());
                new (b.m_valueUnion.error) Unexpected<E>(error());
                new (a.m_valueUnion.value) T(tmp);
                b.has_value = false;
                a.has_value = true;
            }
            else if (a.has_value())
            {
                auto tmp = std::move(a.value());
                new (a.m_valueUnion.error) Unexpected<E>(b.error());
                new (b.m_valueUnion.value) T(tmp);
                b.has_value = true;
                a.has_value = false;
            }
            else
            {
                std::swap(*a.error_pointer(), *b.error_pointer());
            }
        }

        /**
         * Returns the expected value of the Expected if it exists, or the
         * passed default_value if it does not.
         * default_value is a forwarding reference that must be convertible to
         * the type of value stored.
        */
        template<typename U>
        constexpr T value_or(U&& default_value) const&
        {
            return has_value() ? **this : static_cast<T>(std::forward<U>(default_value));
        }

        /**
         * Returns the expected value of the Expected if it exists, or the
         * passed default_value if it does not.
         * default_value is a forwarding reference that must be convertible to
         * the type of value stored.
        */
        template<typename U>
        constexpr T value_or(U&& default_value) &&
        {
            return has_value() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
        }

        bool has_value() const
        {
            return m_hasValue;
        }

        explicit operator bool() const
        {
            return has_value();
        }

        T& value() &
        {
            return *value_pointer();
        }

        T&& value() &&
        {
            return std::move(*value_pointer());
        }

        T& operator*() &
        {
            return value();
        }

        T&& operator*() &&
        {
            return std::move(value());
        }

        T& operator->()
        {
            return value_pointer();
        }

        E& error() &
        {
            return *error_pointer();
        }

        E&& error() &&
        {
            return std::move(*error_pointer());
        }

    private:
        union ValueUnion
        {
            alignas(T) uint8_t value[sizeof(T)];
            alignas(Unexpected<E>) uint8_t error[sizeof(Unexpected<E>)];
        };

        ValueUnion m_valueUnion;

        bool m_hasValue { false };

        T* value_pointer()
        {
            KERNEL_ASSERT(has_value());
            return std::launder(reinterpret_cast<T*>(m_valueUnion.value));
        }

        E* error_pointer()
        {
            KERNEL_ASSERT(!has_value());
            return std::launder(reinterpret_cast<E*>(m_valueUnion.error));
        }

        void reset()
        {
            if (m_hasValue) {
                auto* value = reinterpret_cast<T*>(m_valueUnion.value);
                value->~T();
            } else {
                auto* error = reinterpret_cast<Unexpected<E>*>(m_valueUnion.error);
                error->~Unexpected<E>();
            }
        }
    };
}

#endif
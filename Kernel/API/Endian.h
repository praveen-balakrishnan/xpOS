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

#ifndef XPOS_ENDIAN_H
#define XPOS_ENDIAN_H

#include <bit>

namespace Utilities
{
    template<typename T>
    class [[gnu::packed]] BigEndian
    {
    public:
        constexpr BigEndian() = default;

        constexpr BigEndian(T value)
        {
            set_value(value);
        }

        constexpr T operator=(T value) 
        {
            m_value = std::byteswap(value);
            return value;
        }

        constexpr T get_value() const
        {
            return std::byteswap(m_value);
        }

        constexpr void set_value(T value)
        {
            m_value = std::byteswap(value);
        }

        constexpr void set_raw_value(T value)
        {
            m_value = value;
        }

        friend bool operator== (const BigEndian& lhs, const BigEndian& rhs)
        {
            return lhs.get_value() == rhs.get_value();
        }

    private:
        T m_value;
    };
}

#endif

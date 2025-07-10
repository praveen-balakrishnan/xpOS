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

#ifndef XPOS_COMMON_STRING_H
#define XPOS_COMMON_STRING_H

#include <cstdint>
#include "Memory/Memory.h"

char* strcpy(char* dst, const char* str);
char* strncpy(char* dst, const char* str, uint64_t num);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, uint64_t num);

namespace Common
{

class HashableString
{
public:
    HashableString(const char* str)
    {
        memset(m_str, 0, STRING_LENGTH);
        strcpy(m_str, str); 
    }

    friend bool operator==(const HashableString& lhs, const HashableString& rhs)
    {
        return !strcmp(lhs.m_str, rhs.m_str);
    }

    const char* string()
    {
        return m_str;
    }
private:
    static constexpr int STRING_LENGTH = 256;
    char m_str[STRING_LENGTH];
};

}
#endif
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

#include "Common/String.h"

char* strcpy(char* dst, const char* str)
{
    char* dptr = dst;
    while ((*dptr++=*str++));
    return dst;
}

char* strncpy(char* dst, const char* str, uint64_t num)
{
    char* dptr = dst;
    while ((*dptr++=*str++)&&(--num));
    for (; num-- ; *dst++ = '\0');
    return dst;
}

int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const uint8_t*)s1 - *(const uint8_t*)s2;
}
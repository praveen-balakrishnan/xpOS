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

#include "icxxabi.h"
 
#ifdef __cplusplus
extern "C" {
#endif

void *__dso_handle = 0;
 
int __cxa_atexit(void (*f)(void *), void *objptr, void *dso)
{
	return 0;
};

#ifdef __cplusplus
};
#endif
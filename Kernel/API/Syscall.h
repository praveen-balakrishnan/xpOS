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

#ifndef XPOS_API_SYSCALL_H
#define XPOS_API_SYSCALL_H

#include <cstdint>

#define SYSCALL_DPRINTF 1
#define SYSCALL_VMMAP 2
#define SYSCALL_VMUNMAP 3
#define SYSCALL_POPEN 4
#define SYSCALL_PCLOSE 5
#define SYSCALL_PREAD 6
#define SYSCALL_PWRITE 7
#define SYSCALL_PSEEK 18
#define SYSCALL_NSOCK_BIND 8
#define SYSCALL_NSOCK_LISTEN 9
#define SYSCALL_NSOCK_ACCEPT 10
#define SYSCALL_NSOCK_CONNECT 11
#define SYSCALL_LSOCK_BIND 19
#define SYSCALL_LSOCK_LISTEN 20
#define SYSCALL_LSOCK_ACCEPT 21
#define SYSCALL_LSOCK_CONNECT 22
#define SYSCALL_ELISTENER_ADD 12
#define SYSCALL_ELISTENER_REMOVE 13
#define SYSCALL_SLEEP_FOR 15
#define SYSCALL_GETTID 16
#define SYSCALL_SETFSBASE 17
#define SYSCALL_DNUMPRINT 23
#define SYSCALL_LAUNCH_THREAD 24
#define SYSCALL_FUTEX_WAKE 25
#define SYSCALL_FUTEX_WAIT 26
#define SYSCALL_PINFO 27
#define SYSCALL_BOOTTICKS_MS 28

namespace xpOS::API::Syscalls
{

static uint64_t syscall(uint64_t num)
{
    uint64_t ret;
    asm volatile("mov %1, %%rax\n"
                    "syscall\n"
                    "mov %%rax, %0\n" : "=r"(ret) : "r" (num));
    return ret;
}

static uint64_t syscall(uint64_t num, uint64_t arg1)
{
    uint64_t ret;
    asm volatile("mov %1, %%rax\n" "mov %2, %%rdi\n"
                    "syscall\n"
                    "mov %%rax, %0\n" : "=r"(ret) : "r" (num), "r" (arg1): "rdi");
    return ret;
}

static uint64_t syscall(uint64_t num, uint64_t arg1, uint64_t arg2)
{
    uint64_t ret;
    asm volatile("mov %1, %%rax\n" "mov %2, %%rdi\n" "mov %3, %%rsi\n"
                    "syscall\n"
                    "mov %%rax, %0\n" : "=r"(ret) : "r" (num), "r" (arg1), "r" (arg2): "rdi", "rsi");
    return ret;
}

static uint64_t syscall(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    uint64_t ret;
    asm volatile("mov %1, %%rax\n" "mov %2, %%rdi\n" "mov %3, %%rsi\n" "mov %4, %%rdx\n"
                    "syscall\n"
                    "mov %%rax, %0\n" : "=r"(ret) : "r" (num), "r" (arg1), "r" (arg2), "r" (arg3) : "rdi", "rsi", "rdx");
    return ret;
}

static uint64_t syscall(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    uint64_t ret;
    asm volatile("mov %1, %%rax\n" "mov %2, %%rdi\n" "mov %3, %%rsi\n" "mov %4, %%rdx\n" "mov %5, %%r10\n"
                    "syscall\n"
                    "mov %%rax, %0\n" : "=r"(ret) : "r" (num), "r" (arg1), "r" (arg2), "r" (arg3), "r" (arg4) : "rdi", "rsi", "rdx", "r10");
    return ret;
}

static uint64_t syscall(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    uint64_t ret;
    asm volatile("mov %1, %%rax\n" "mov %2, %%rdi\n" "mov %3, %%rsi\n" "mov %4, %%rdx\n" "mov %5, %%r10\n" "mov %6, %%r8\n"
                    "syscall\n"
                    "mov %%rax, %0\n" : "=r"(ret) : "r" (num), "r" (arg1), "r" (arg2), "r" (arg3), "r" (arg4), "r" (arg5): "rdi", "rsi", "rdx", "r10", "r8");
    return ret;
}

static uint64_t syscall(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    uint64_t ret;
    asm volatile("mov %1, %%rax\n" "mov %2, %%rdi\n" "mov %3, %%rsi\n" "mov %4, %%rdx\n" "mov %5, %%r10\n" "mov %6, %%r8\n" "mov %7, %%r9\n"
                    "syscall\n"
                    "mov %%rax, %0\n" : "=r"(ret) : "r" (num), "r" (arg1), "r" (arg2), "r" (arg3), "r" (arg4), "r" (arg5), "r" (arg6): "rdi", "rsi", "rdx", "r10", "r8", "r9");
    return ret;
}

}

namespace xpOS::API
{

static void kern_print(const char* str)
{
    Syscalls::syscall(SYSCALL_DPRINTF, reinterpret_cast<uint64_t>(str));
}

static void kern_print_num(uint64_t c)
{
    Syscalls::syscall(SYSCALL_DNUMPRINT, c);
}

}

#endif
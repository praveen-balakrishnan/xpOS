;
;    Copyright 2023-2025 Praveen Balakrishnan
;
;    Licensed under the Apache License, Version 2.0 (the "License");
;    you may not use this file except in compliance with the License.
;    You may obtain a copy of the License at
;
;        http://www.apache.org/licenses/LICENSE-2.0
;
;    Unless required by applicable law or agreed to in writing, software
;    distributed under the License is distributed on an "AS IS" BASIS,
;    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;    See the License for the specific language governing permissions and
;    limitations under the License.
;
;    xpOS v1.0
;

global long_mode_start
extern call_constructors
extern pre_kernel
extern stackTop
extern gdt64.pointer
extern gdt64
extern tss64
section .text
bits 64

long_mode_start:
    cli
    ; The kernel is loaded in to the higher half of virtual memory, so we begin
    ; executing code from here.
    mov rax, higher_half_switch
    jmp rax
higher_half_switch:
    mov rsp, stackTop
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ;call call_constructors
    mov rdi, 0
    mov edi, ebx
    ; We pass the structure pointers onto the pre_kernel.
    mov rsi, gdt64.pointer 
    lgdt [rsi]
    mov rsi, gdt64
    mov rdx, tss64
	call pre_kernel 
    hlt
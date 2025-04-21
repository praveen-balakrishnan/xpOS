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

bits 64
section .text
global isr_def_32
global send_pit_eoi
global switch_to_not_started_task
global switch_to_ready_task

extern _ZN6X86_6425ProgrammableIntervalTimer4tickEv
extern _ZN4Task11TaskManager12task_cleanupEv

%macro pushaq 0
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Save FS_BASE to the stack.
    push rcx
    push rdx
    push rax
    mov ecx, 0xC0000100
    rdmsr
    shl rdx, 32
    or rdx, rax
    mov rbx, rdx
    pop rax
    pop rdx
    pop rcx
    push rbx
    
    ; Save the SSE/MMX state on the stack, taking care
    ; to ensure it is 16 bit aligned.
    mov r12, rsp
    sub rsp, 544
    mov r13, rsp     
    add r13, 16  
    add r13, 15 
    and r13, -16

    mov qword [rsp],   r12
    mov qword [rsp+8], r13

    fxsave [r13]

%endmacro

%macro popaq 0
    ; Restore the SSE/MMX state.
    mov r13, qword [rsp+8]
    fxrstor [r13]
    mov rsp, qword [rsp] 

    ; Restore the FS_BASE.
    pop rbx  
    push rax
    push rcx
    push rdx
    mov ecx, 0xC0000100
    mov eax, ebx
    mov rdx, rbx
    shr rdx, 32
    wrmsr 
    pop rdx
    pop rcx
    pop rax
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
%endmacro

isr_def_32:
    pushaq
    ; X86:64::ProgrammableIntervalTimer::tick()
    call _ZN6X86_6425ProgrammableIntervalTimer4tickEv
    popaq
    iretq

switch_to_not_started_task:
    pushaq
    cmp rcx, 0
    je .skip_save_stack
    mov [rcx], rsp
.skip_save_stack:
    mov rsp, [rdi]
    mov rcx, cr3
    cmp rsi, rcx
    je .virtaddrspace_changed
    mov cr3, rsi
.virtaddrspace_changed:
    sti
    push r8
    mov rdi, r9
    jmp rdx


switch_to_ready_task:
    pushaq
    cmp rdx, 0
    je .skip_save_stack
    mov [rdx], rsp
.skip_save_stack:
    mov rsp, [rdi]
    mov rcx, cr3
    cmp rsi, rcx
    je .virtaddrspace_changed
    mov cr3, rsi
.virtaddrspace_changed:
    popaq
    sti
    ret

send_pit_eoi:
    mov al, 0x20
    out 0x20, al
    ret
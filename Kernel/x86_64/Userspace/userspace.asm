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

global asm_flush_tss
global enable_syscall_sysret
global start_userspace_task_sysret
global start_userspace_task_iretq
extern _ZN9Processes15syscall_handlerEmmmmmm
bits 64
section .text

; Load TSS selector
asm_flush_tss: 
    mov ax, 0x28
	ltr ax
	ret

asm_syscall_handler:
	push rcx
	push r11
	; We preserve rcx for sysret, so we can clobber it for arguments.
	mov rcx, r10
	call _ZN9Processes15syscall_handlerEmmmmmm
	pop r11
	pop rcx
	o64 sysret

enable_syscall_sysret:
	mov rcx, 0xc0000080
	rdmsr
	; Enable syscall/sysret.
	or eax, 1 
	wrmsr
	mov rcx, 0xc0000081 
	rdmsr
	; Load code/stack segments for PL0/3 into IA32_STAR.
	mov edx, 0x00100008 
	wrmsr
	mov rcx, 0xc0000082
	mov rdi, asm_syscall_handler
	; Load syscall handler into IA32_LSTAR.
	mov eax, edi 
	shr rdi, 32
	mov edx, edi
	wrmsr
	ret

; Fake a syscall stack to sysret into ring 3.
start_userspace_task_sysret: 
	mov rcx, rdi
	; Enable interrupts in RFLAGS.
	mov r11, 0x202
	mov rsp, rsi
	o64 sysret

; Fake an interrupt stack to iret into ring 3.
start_userspace_task_iretq: 
	; User-space data selector.
	push (3*8)|3
	; Target stack pointer.
	push rsi 
	; Enable interrupts in RFLAGS.
	push 0x202
	; User-space code selector.
	push (4*8)|3
	; Target instruction pointer.
	push rdi
	iretq

start_kernel_task:
	mov rsp, rsi
	call rdi
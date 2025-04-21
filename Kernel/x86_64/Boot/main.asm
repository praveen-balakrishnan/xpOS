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

section .text
global _start
global stackTop
global gdt64.pointer
global gdt64
global tss64
extern long_mode_start
bits 32 

; This is the entry point of the kernel from the bootloader.
;
; This initialisation code is specific to the Intel 64/x86-64/AMD64
; architecture. Refer to the Intel® 64 and IA-32 Architectures Software
; Developer's Manual, Volume 3.
;
; We set up the required structures to enter 64-bit execution (long mode) from
; protected mode.

KERNEL_V_BASE               EQU 0xFFFFFF8000000000

PAGE_PRESENT                EQU 0x00000001
PAGE_WRITABLE               EQU 0x00000002
PAGE_HUGE                   EQU 0x00000080
PAGE_4KiB                   EQU 0x00001000
PAGE_2MiB                   EQU 0x00200000
PAGE_1GiB                   EQU 0x40000000

EFLAGS_ID                   EQU 1 << 21

CPUID_VERSION_INFO          EQU 0x00000001
CPUID_MAX_EXT_FUNC          EQU 0x80000000
CPUID_EXT_PROC_SIG_FEATURE  EQU 0x80000001

CPUID_SSE                   EQU 1 << 25
CPUID_LONG_MODE             EQU 1 << 29

CR0_MP                      EQU 1 << 1
CR0_EM                      EQU 1 << 2
CR0_PG                      EQU 1 << 31

CR4_PAE                     EQU 1 << 5
CR4_OSFXSR                  EQU 1 << 9
CR4_OSXMMEXCPT              EQU 1 << 10

EFER_MSR                    EQU 0xC0000080
EFER_LME                    EQU 1 << 8

SEGMENT_PRESENT             EQU 1 << 7
SEGMENT_USER                EQU 3 << 5
SEGMENT_CODE                EQU 0b11010
SEGMENT_DATA                EQU 0b10010
SEGMENT_FLAG_MAX_LIMIT      EQU 0b1111
SEGMENT_AVL                 EQU 1 << 4
SEGMENT_LONG_MODE_CODE      EQU 1 << 5
SEGMENT_DEFAULT             EQU 1 << 6
SEGMENT_GRANULARITY         EQU 1 << 7
SEGMENT_MAX_LIMIT           EQU 0xFFFF

_start: 
    mov esp, stackTop - KERNEL_V_BASE
    ; The Multiboot2 structure is located in the EBX register, so we preserve
    ; it so we can clobber the register.
    push ebx

    call check_cpu
    call init_paging_and_long_mode
    call enable_sse

    pop ebx

    ; We perform a far jump to the long mode entry point. This is to the lower
    ; half address.
    jmp gdt64.kernelCodeSegment:long_mode_start - KERNEL_V_BASE
halt_end:
    hlt

; Check that the CPU supports the minimum feature set required for xpOS.
check_cpu:
    ; Check that CPUID is supported by flipping EFLAGS.ID and seeing if the
    ; flip is maintained.
    pushfd
    pushfd
    xor dword [esp], EFLAGS_ID
    popfd
    pushfd
    pop eax
    xor eax, [esp]
    popfd
    cmp eax, 0
    je cpu_arch_error

    ; Check that SSE is supported.
    mov eax, CPUID_VERSION_INFO
    cpuid
    test edx, CPUID_SSE
    jz cpu_arch_error

    ; Check that CPUID 0x80000001 is supported.
    mov eax, CPUID_MAX_EXT_FUNC
    cpuid
    cmp eax, CPUID_EXT_PROC_SIG_FEATURE
    jb cpu_arch_error

    ; Check that long mode is supported.
    mov eax, CPUID_EXT_PROC_SIG_FEATURE
    cpuid
    and edx, CPUID_LONG_MODE
    cmp edx, CPUID_LONG_MODE
    jne cpu_arch_error

    ret

; Enable streaming SIMD extensions (SSE).
enable_sse:
    mov eax, cr0
    ; Disable coprocessor emulation and enable coprocessor monitoring.
    and ax, 0xFFFF - CR0_EM
    or ax, CR0_MP
    mov cr0, eax
    mov eax, cr4
    ; Enable FXSAVE/FXRSTOR instructions and unmasked SSE exceptions.
    or ax, CR4_OSFXSR | CR4_OSXMMEXCPT
    mov cr4, eax

    ret

;setup_page_tables:
;    mov eax, PDPT
;    or eax, PAGE_PRESENT | PAGE_WRITABLE
;    ; We identity map 512GiB of memory at the start of address space.
;    mov [PML4T], eax
;    ; We also map it to the last 512GiB of the address space (KERNEL_V_BASE).
;    mov [PML4T + 511 * 8], eax
;    mov ecx, 0
;    .loop:
;     mov eax, PAGE_1GiB 
;     mul ecx
;     or eax, PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE
;     mov edx, PDPT
;     mov [edx + ecx * 8], eax
;     inc ecx
;     cmp ecx, 1
;     jne .loop
;    ret

setup_page_tables:
    mov eax, PDPT
    or eax, PAGE_PRESENT | PAGE_WRITABLE
    ; We identity map 512GiB of memory at the start of address space.
    mov [PML4T], eax
    ; We also map it to the last 512GiB of the address space (KERNEL_V_BASE).
    mov [PML4T + 511 * 8], eax
    mov eax, PDT
    or eax, PAGE_PRESENT | PAGE_WRITABLE
    mov [PDPT], eax
    mov ecx, 0
    .loop:
     mov eax, PAGE_2MiB 
     mul ecx
     or eax, PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE
     mov edx, PDT
     mov [edx + ecx * 8], eax
     inc ecx
     cmp ecx, 512
     jne .loop
    ret


init_paging_and_long_mode: 
    call setup_page_tables
    mov eax, PML4T
    ; Load the top level page table address for paging.
    mov cr3, eax
    mov eax, cr4
    ; Enable Physical Address Extension paging.
	or eax, CR4_PAE
	mov cr4, eax

    mov ecx, EFER_MSR
	rdmsr
    ; Enable long mode.
	or eax, EFER_LME
	wrmsr

    mov eax, cr0
    ; Enable virtual memory paging.
	or eax, CR0_PG
	mov cr0, eax
    ; Load the Global Descriptor Table.
    mov ebx, gdt64.pointer - KERNEL_V_BASE
    lgdt [ebx]
    ret

cpu_arch_error:
    ; We print an error message to the screen by writing to the video memory
    ; for default text mode. Each ASCII text character is followed by a colour
    ; code in the video memory.
    VRAM_BASE EQU 0xB8000
    TEXT_COLOR EQU 0x4F
    mov esi, errorMsg
    mov edi, VRAM_BASE
    .loop:
     cmp byte [esi], 0
     je .endloop
     mov al, byte [esi]
     mov byte [edi], al
     inc edi
     mov byte [edi], byte TEXT_COLOR
     inc edi
     inc esi
     jmp .loop
    .endloop:
    hlt

section .data
; The Task State Segment in 64-bit mode is used primarily to store stack
; pointers for different privilege levels. It is initialised later in the
; pre-kernel.
tss64:
    times 104 db 0

; The Global Descriptor Table is used to store different segments in the
; segmentation model used by the x86 architecture. We use it to store segments
; for different protection levels with different read/write/execute
; permissions. Namely, we have a code segment and a data segment for both ring 
; 0 (kernel) and ring 3 (user).

struc GDTEntry
    .segmentLimit:  resw 1
    .baseAddrLow:   resw 1
    .baseAddrMid:   resb 1
    .accessBits:    resb 1
    .flagBits:      resb 1
    .baseAddrHigh:  resb 1
endstruc

gdt64:
; The first entry of the GDT must be zero.
.zeroEntry:
	dq 0
.kernelCodeSegment: equ $ - gdt64
    istruc GDTEntry
        at GDTEntry.segmentLimit,   dw SEGMENT_MAX_LIMIT
        at GDTEntry.baseAddrLow,    dw 0
        at GDTEntry.baseAddrMid,    db 0
        at GDTEntry.accessBits,     db SEGMENT_PRESENT | SEGMENT_CODE
        at GDTEntry.flagBits,       db SEGMENT_GRANULARITY | SEGMENT_LONG_MODE_CODE | SEGMENT_FLAG_MAX_LIMIT
        at GDTEntry.baseAddrHigh,   db 0
    iend
.kernelDataSegment: equ $ - gdt64
    istruc GDTEntry
        at GDTEntry.segmentLimit,   dw SEGMENT_MAX_LIMIT
        at GDTEntry.baseAddrLow,    dw 0
        at GDTEntry.baseAddrMid,    db 0
        at GDTEntry.accessBits,     db SEGMENT_PRESENT | SEGMENT_DATA
        at GDTEntry.flagBits,       db SEGMENT_GRANULARITY | SEGMENT_DEFAULT | SEGMENT_FLAG_MAX_LIMIT
        at GDTEntry.baseAddrHigh,   db 0
    iend
.userspaceDataSegment: equ $ - gdt64
    istruc GDTEntry
        at GDTEntry.segmentLimit,   dw SEGMENT_MAX_LIMIT
        at GDTEntry.baseAddrLow,    dw 0
        at GDTEntry.baseAddrMid,    db 0
        at GDTEntry.accessBits,     db SEGMENT_PRESENT | SEGMENT_USER | SEGMENT_DATA
        at GDTEntry.flagBits,       db SEGMENT_GRANULARITY | SEGMENT_DEFAULT | SEGMENT_FLAG_MAX_LIMIT
        at GDTEntry.baseAddrHigh,   db 0
    iend
.userspaceCodeSegment: equ $ - gdt64
    istruc GDTEntry
        at GDTEntry.segmentLimit,   dw SEGMENT_MAX_LIMIT
        at GDTEntry.baseAddrLow,    dw 0
        at GDTEntry.baseAddrMid,    db 0
        at GDTEntry.accessBits,     db SEGMENT_PRESENT | SEGMENT_USER | SEGMENT_CODE
        at GDTEntry.flagBits,       db SEGMENT_GRANULARITY | SEGMENT_LONG_MODE_CODE | SEGMENT_FLAG_MAX_LIMIT
        at GDTEntry.baseAddrHigh,   db 0
    iend
; A descriptor entry for the TSS is stored in the GDT. 
.taskStateSegment: equ $ - gdt64
    dq 0
    dq 0
.pointer:
    ; length
	dw $ - gdt64 - 1 
    ; address
	dq gdt64 

errorMsg: equ $ - KERNEL_V_BASE
    db "ERROR: UNSUPPORTED CPU.",0

section .bss
align 0x1000
PML4T: equ $ - KERNEL_V_BASE
	resb PAGE_4KiB
PDPT: equ $ - KERNEL_V_BASE
    resb PAGE_4KiB
PDT: equ $ - KERNEL_V_BASE
    resb PAGE_4KiB
; Reserve 32KiB for the stack 
stackBottom:
    resb 0x100000
stackTop:
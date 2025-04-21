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

section .multiboot_header
header_start:
    ; Magic number to identify the Multiboot2 header.
    dd 0xe85250d6
    ; i386 32-bit architecture.
    dd 0
    ; Length of Multiboot2 header.
    dd header_end - header_start 
    ; Checksum set so sum of fields is 0.
    dd 0x100000000 - (0xe85250d6 + (header_end - header_start)) 

    ; End tag.
    dw 0
    dw 0
    dd 8
header_end:

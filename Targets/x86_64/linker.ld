/*
    COPYRIGHT (c) 2022-2023 Praveen Balakrishnan
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    xpOS v1.0.0
*/

ENTRY(_start)

kernel_v_base = 0xFFFFFF8000000000;

SECTIONS
{
	. = 1M;

	start_of_kernel_image = .;

	.multiboot_header :
	{
        *(.multiboot_header)
    }

	. += kernel_v_base;

	.text ALIGN(4K) : AT (ADDR (.text) - kernel_v_base)
	{
		*(.text)
	}
 
	.rodata ALIGN(4K) : AT (ADDR (.rodata) - kernel_v_base)
	{
		*(.rodata)
	}
 
	.data ALIGN(4K) : AT (ADDR (.data) - kernel_v_base)
	{
		/*start_ctors = .;
		KEEP(*( .init_array ));
    	KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
		end_ctors = .;*/
		PROVIDE_HIDDEN (__ctors_array_start = .);
		KEEP (*(.ctors))
		PROVIDE_HIDDEN (__ctors_array_end = .);
		*(.data)
	}
 
	.bss ALIGN(4K): AT (ADDR (.bss) - kernel_v_base)
	{
		*(.bss*)
	}

	end_of_kernel_image = .;
}
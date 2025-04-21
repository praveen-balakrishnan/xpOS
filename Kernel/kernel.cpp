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

#include "Arch/CPU.h"
#include "Arch/Interrupts/Interrupts.h"
#include "Arch/IO/PCI.h"
#include "Arch/IO/PIT.h"
#include "Boot/Modules/ModuleManager.h"
#include "Boot/MultibootManager.h"
#include "Drivers/Graphics/VMWare/SVGAII.h"
#include "Drivers/HID/Keyboard.h"
#include "Drivers/HID/Mouse.h"
#include "Drivers/Initrd/TARinitrd.h"
#include "Filesystem/VFS.h"
#include "Loader/ELF.h"
#include "Memory/MemoryManager.h"
#include "Memory/SharedMemory.h"
#include "Networking/NetworkSocket.h"
#include "Pipes/LocalSocket.h"
#include "Pipes/EventListener.h"
#include "Pipes/Pipe.h"
#include "Tasks/TaskManager.h"
#include "Tasks/MasterScheduler.h"

//#include "guard-abi.h"
#include "print.h"
#include "x86_64.h"

extern "C" void __cxa_pure_virtual()
{
    printf("This should not be called.");
}

typedef void (*constructor)();
extern "C" uint64_t start_of_kernel_image;
extern "C" uint64_t end_of_kernel_image;
extern "C" char __ctors_array_start;
extern "C" char __ctors_array_end;

void call_constructors()
{
    auto start = reinterpret_cast<uint64_t*>(&__ctors_array_start);
    auto end = reinterpret_cast<uint64_t*>(&__ctors_array_end);
    
    auto it = start;

    while(it != end) {
        auto fn = reinterpret_cast<void(*)()>(*it);
        (*fn)();
        ++it;
    }
}

extern "C" void jump_to_usermode_entry();
extern "C" void start_userspace_task_iretq(uint64_t entryPoint, uint64_t userStackPointer);
extern "C" void start_userspace_task_sysret(void* entryPoint, uint64_t userStackPointer);

void launch_bootstrap(const char* executable)
{
    auto currentTask = Task::Manager::instance().get_current_task();
    Loader::prepare_elf(executable, currentTask);
    auto entryPoint = currentTask->entryPoint;
    start_userspace_task_sysret(entryPoint, Loader::START_STACK_PTR);
}

void window_server_launch()
{
    launch_bootstrap("xpinitrd/System/WindowServer");
}

extern "C" void pre_kernel(void* multibootStructure,
                            void* gdtPtr, void* tssPtr)
{
    clear_screen_and_init_serial();
    call_constructors();

    auto multibootVirtualAddress = Memory::VirtualAddress(Memory::PhysicalAddress(multibootStructure));
    Multiboot::set_structure(static_cast<Multiboot::multiboot_info*>(multibootVirtualAddress.get()));

    auto endOfKernel = Memory::VirtualAddress(reinterpret_cast<uint64_t>(&end_of_kernel_image));
    Memory::Manager::instance().protect_physical_regions(endOfKernel.get_low_physical());
    Modules::protect_pages();

    Memory::Manager::instance().remap_pages();
    Modules::map_virtual();

    auto* gdt = static_cast<X86_64::GlobalDescriptorTable*>(Memory::VirtualAddress(gdtPtr).get());
    auto* tss = static_cast<X86_64::TaskStateSegment*>(Memory::VirtualAddress(tssPtr).get());

    CPU::set_global_descriptor_table(gdt);
    CPU::set_task_state_segment(tss);

    X86_64::Interrupts::Manager::instance().initialise();
    X86_64::ProgrammableIntervalTimer::instance().initialise();

    //Memory::Heap::HeapManager::instance();

    Pipes::initialise();
    Task::MasterScheduler sch; 
    Task::Manager::instance().initialise(&sch);
    //Events::EventDispatcher::instance();
    PCI::initialise();
    Sockets::LocalSocket::initialise();
    Networking::NetworkSocket::initialise();
    Pipes::EventListener::initialise();
    
    Filesystem::VirtualFilesystem::instance();
    Filesystem::TarReader::instance().load_initrd();

    auto newAddrSpace = Memory::Manager::instance().create_virtual_address_space();

    Task::TaskDescriptor tdesc1
    {
        .addressSpace = Memory::RegionableVirtualAddressSpace(std::move(newAddrSpace)),
        .openPipes = Common::AutomaticReferenceCountable<Task::PipeTable>(Task::PipeTable()),
        .entryPoint = (void*)&window_server_launch,
        .taskPriority = 300
    };

    Devices::HID::Keyboard::initialise();
    Devices::HID::Mouse::initialise();
    Drivers::Graphics::VMWareSVGAII::Device::instance().initialise();
    Memory::Shared::initialise();
    
    Task::Manager::instance().launch_task(tdesc1);
    
    Task::Manager::instance().begin();
}
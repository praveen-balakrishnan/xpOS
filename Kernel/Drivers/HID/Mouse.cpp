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

#include "API/HID.h"
#include "Arch/Interrupts/Interrupts.h"
#include "Arch/IO/IO.h"
#include "Common/CircularBuffer.h"
#include "Common/List.h"
#include "Drivers/HID/Mouse.h"

namespace Devices::HID::Mouse
{
    using MouseBuffer = Common::FixedCircularBuffer<30>;
    using namespace xpOS;

    namespace {
        Common::List<MouseBuffer*> buffers;
        Pipes::EventListenerList eventListenerList;
        Mutex bufferListLock;
        Mutex eventListenerLock;
        Spinlock mouseQueueLock;
        Common::List<API::HID::MouseEvent> queuedMessages;
        uint64_t threadId = 0;
        Pipes::DeviceOperations deviceOperations;
        uint8_t buffer[3];
        int bufferPointer = 0;
    }

    void receive_thread(void*)
    {
        mouseQueueLock.acquire();
        while (true) {
            if (queuedMessages.size() > 0) {
                for (auto message : queuedMessages) {
                    for (auto buffer : buffers)
                        buffer->write(sizeof(API::HID::MouseEvent), &message);
                }
                queuedMessages.clear();
                eventListenerList.notify(Pipes::EventTypes::READABLE);
            }
            mouseQueueLock.release();
            Task::Manager::instance().about_to_block();
            Task::Manager::instance().block();
            mouseQueueLock.acquire();
        }
    }

    void irq_handler()
    {
        // This is called by the interrupt handler, so we only put the scancode
        // in a queue to be processed.
        uint8_t status = IO::in_8(COMMAND_PORT);
        // The byte did not originate from the mouse, so do not look at it.
        if (!(status & 0x20))
            return;

        buffer[bufferPointer] = IO::in_8(DATA_PORT);
        
        bufferPointer = (bufferPointer + 1) % 3;

        if (bufferPointer == 0) {
            API::HID::MouseEvent mevent;
            mevent.buttons = buffer[0] & MOUSE_BUTTONS_MASK;
            if (buffer[1] != 0 || buffer[2] != 0) {
                mevent.dx = buffer[1];
                mevent.dy = buffer[2];
                if (buffer[0] & 0x10)
                    mevent.dx -= 0x100;
                if (buffer[0] & 0x20)
                    mevent.dy -= 0x100;
                mevent.dy = -mevent.dy;
            }
            if (threadId && Task::Manager::instance().is_executing()) {
                mouseQueueLock.acquire();
                queuedMessages.push_back(mevent);
                mouseQueueLock.release();
                Task::Manager::instance().unblock(threadId);
            }
        }

    }

    void initialise()
    {
        IO::out_8(COMMAND_PORT, 0xA8);
        IO::out_8(COMMAND_PORT, 0x20);
        uint8_t status = IO::in_8(DATA_PORT);
        status |= 2;
        IO::out_8(COMMAND_PORT, 0x60);
        IO::out_8(DATA_PORT, status);

        IO::out_8(COMMAND_PORT, 0xD4);
        IO::out_8(DATA_PORT, 0xF4);
        IO::in_8(DATA_PORT);

        add_interrupt_handler(irq_handler, IRQ_LINE);
        threadId = Task::Manager::instance().launch_kernel_process((void*)&receive_thread, 0);
        deviceOperations = {
            .open = open,
            .close = close,
            .read = read,
            .notify = notify,
            .denotify = denotify
        };
        Pipes::register_device("hid::mouse", &deviceOperations);
    }

    bool open(void* with, int flags, void*& deviceSpecific)
    {
        auto* mouseBuffer = new MouseBuffer();
        deviceSpecific = mouseBuffer;
        LockAcquirer l(bufferListLock);
        buffers.push_back(mouseBuffer);
        return true;
    }

    void close(void*& deviceSpecific)
    {
        auto* mouseBuffer = reinterpret_cast<MouseBuffer*>(deviceSpecific);
        for (auto it = buffers.begin(); it != buffers.end(); it++) {
            if (*it == mouseBuffer) {
                buffers.erase(it);
                break;
            }
        }
        delete mouseBuffer;
    }

    std::size_t read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific)
    {
        auto* mouseBuffer = reinterpret_cast<MouseBuffer*>(deviceSpecific);
        LockAcquirer l(bufferListLock);
        return mouseBuffer->read(count, buf);
    }

    Pipes::EventListenerList::Receipt notify(void* listener, Pipes::raise_events_callback raise_event, Pipes::EventTypeMask& current, void*& deviceSpecific)
    {
        auto* mouseBuffer = reinterpret_cast<MouseBuffer*>(deviceSpecific);
        Pipes::EventTypeMask mask = 0;
        {
            LockAcquirer l(bufferListLock);
            if (!mouseBuffer->is_queue_empty())
                mask |= Pipes::EventTypes::READABLE;
        }

        LockAcquirer l(eventListenerLock);
        return eventListenerList.add(listener, raise_event);
    }

    void denotify(Pipes::EventListenerList::Receipt receipt, void*& deviceSpecific)
    {
        LockAcquirer l(eventListenerLock);
        return eventListenerList.remove(receipt);
    }
}
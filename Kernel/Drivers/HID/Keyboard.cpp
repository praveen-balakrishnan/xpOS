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

#include "Arch/Interrupts/Interrupts.h"
#include "Arch/IO/IO.h"
#include "Common/CircularBuffer.h"
#include "Drivers/HID/Keyboard.h"

namespace Devices::HID::Keyboard
{
    using ScancodeBuffer = Common::FixedCircularBuffer<30>;
    using namespace xpOS::API::HID;
    
    namespace {
        // A list of buffers populated with scancodes for each listener.
        Common::List<ScancodeBuffer*> buffers;
        Pipes::EventListenerList eventListenerList;
        Mutex bufferListLock;
        Mutex eventListenerLock;
        Spinlock scancodeQueueLock;
        Common::List<uint8_t> queuedMessages;
        Common::List<KeyboardEvent> keyEvents;
        uint64_t threadId = 0;
        Pipes::DeviceOperations deviceOperations;
    }

    void convert_keys(Common::List<uint8_t>& scancodes, Common::List<KeyboardEvent>& eventsOut)
    {
        // Some keys have a two byte extended scancode, so we track this property.
        static bool extended = false;
        for (auto it = queuedMessages.begin(); it != queuedMessages.end(); it++) {
            KeyboardEvent k;
            k.key = 0;
            auto scancode = *it;
            if (*it == SCANCODE_S1_EXTID) {
                extended = true;
                continue;
            }
            if (extended) {
                switch (scancode & ~SCANCODE_S1_RELEASEMOD) {
                case SCANCODE_S1_EXTCODES::CURSOR_UP:
                    k.key = ControlKeys::UARROW;
                    break;
                case SCANCODE_S1_EXTCODES::CURSOR_LEFT:
                    k.key = ControlKeys::LARROW;
                    break;
                case SCANCODE_S1_EXTCODES::CURSOR_RIGHT:
                    k.key = ControlKeys::RARROW;
                    break;
                case SCANCODE_S1_EXTCODES::CURSOR_DOWN:
                    k.key = ControlKeys::DARROW;
                    break;
                }
            } else {
                k.key = SCANCODE_S1_MAP[scancode & ~SCANCODE_S1_RELEASEMOD];
            }
            if (scancode & SCANCODE_S1_RELEASEMOD) {
                k.movement = KeyboardEvent::KEY_UP;
            } else {
                k.movement = KeyboardEvent::KEY_DOWN;
            }
            extended = false;
            if (k.key)
                eventsOut.push_back(k);
        }
    }

    void receive_thread(void*)
    {
        scancodeQueueLock.acquire();
        while (true) {
            if (queuedMessages.size() > 0) {
                convert_keys(queuedMessages, keyEvents);
                queuedMessages.clear();
                for (auto message : keyEvents) {
                    for (auto buffer : buffers)
                        buffer->write(sizeof(KeyboardEvent), &message);
                }
                keyEvents.clear();
                eventListenerList.notify(Pipes::EventTypes::READABLE);
            }
            Task::Manager::instance().about_to_block();
            scancodeQueueLock.release();
            Task::Manager::instance().block();
            scancodeQueueLock.acquire();
        }
    }

    void irq_handler()
    {
        // This is called by the interrupt handler, so we only put the scancode
        // in a queue to be processed.
        auto scancode = IO::in_8(SCANCODE_PORT);
        if (threadId && Task::Manager::instance().is_executing()) {
            scancodeQueueLock.acquire();
            queuedMessages.push_back(scancode);
            scancodeQueueLock.release();
            Task::Manager::instance().unblock(threadId);
        }
    }

    void initialise()
    {
        add_interrupt_handler(irq_handler, IRQ_LINE);
        threadId = Task::Manager::instance().launch_kernel_process((void*)&receive_thread, 0);
        deviceOperations = {
            .open = open,
            .close = close,
            .read = read,
            .notify = notify,
            .denotify = denotify
        };
        Pipes::register_device("hid::keyboard", &deviceOperations);
    }

    bool open(void* with, int flags, void*& deviceSpecific)
    {
        auto* scancodeBuffer = new ScancodeBuffer();
        deviceSpecific = scancodeBuffer;
        LockAcquirer l(bufferListLock);
        buffers.push_back(scancodeBuffer);
        return true;
    }

    void close(void*& deviceSpecific)
    {
        auto* scancodeBuffer = reinterpret_cast<ScancodeBuffer*>(deviceSpecific);
        for (auto it = buffers.begin(); it != buffers.end(); it++) {
            if (*it == scancodeBuffer) {
                buffers.erase(it);
                break;
            }
        }
        delete scancodeBuffer;
    }

    std::size_t read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific)
    {
        auto* scancodeBuffer = reinterpret_cast<ScancodeBuffer*>(deviceSpecific);
        LockAcquirer l(bufferListLock);
        return scancodeBuffer->read(count, buf);
    }

    Pipes::EventListenerList::Receipt notify(void* listener, Pipes::raise_events_callback raise_event, Pipes::EventTypeMask& current, void*& deviceSpecific)
    {
        auto* scancodeBuffer = reinterpret_cast<ScancodeBuffer*>(deviceSpecific);
        Pipes::EventTypeMask mask = 0;
        {
            LockAcquirer l(bufferListLock);
            if (!scancodeBuffer->is_queue_empty())
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
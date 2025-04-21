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

#include "Filesystem/VFS.h"

namespace Filesystem
{
    VirtualFilesystem::VirtualFilesystem()
    {
        m_rootNode = new Node("", nullptr);
        m_rootNode->refCount = 1;
        m_deviceOperations = {
            .open = open,
            .close = close,
            .read = read,
            .write = write,
            .info = info
        };
        Pipes::register_device("vfs", &m_deviceOperations);
    }

    int VirtualFilesystem::register_filesystem(ConcreteFilesystemDescriptor filesystem)
    {
        m_rootNode->concreteObject = filesystem.root;
        m_rootNode->concreteObject->operations = filesystem.driver;
        return 1;
    }
    
    void VirtualFilesystem::unregister_filesystem(int mountNum)
    {
        /// FIXME: This should be implemented.
    }

    std::size_t VirtualFilesystem::read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific)
    {
        auto* node = static_cast<Node*>(deviceSpecific);
        return node->concreteObject->operations->read(*node->concreteObject, offset, count, static_cast<char*>(buf));
    }

    std::size_t VirtualFilesystem::write(std::size_t offset, std::size_t count, const void *buf, void*& deviceSpecific)
    {
        auto* node = static_cast<Node*>(deviceSpecific);
        return node->concreteObject->operations->write(*node->concreteObject, offset, count, static_cast<const char*>(buf));
    }

    xpOS::API::Pipes::PipeInfo VirtualFilesystem::info(void*& deviceSpecific)
    {
        auto* node = static_cast<Node*>(deviceSpecific);
        xpOS::API::Pipes::PipeInfo info;
        info.size = node->concreteObject->operations->fileinfo(*node->concreteObject);
        return info;
    }

    std::size_t VirtualFilesystem::filelen(Pipes::Pipe& pipe)
    {
        return reinterpret_cast<Node*>(pipe.device_specific())->concreteObject->operations->fileinfo(*reinterpret_cast<Node*>(pipe.device_specific())->concreteObject);
    }

    bool VirtualFilesystem::open(void* with, int flags, void*& deviceSpecific)
    {
        auto path = static_cast<const char*>(with);

        auto node = VirtualFilesystem::instance().m_rootNode;
        node->lock.acquire();
        auto parsedPath = parse_path(path);

        for (auto pathComponent : parsedPath) {
            Node* nextNode = nullptr;
            for (auto child : node->childNodes) {
                if (!strcmp(child->componentName, pathComponent.str)) {
                    nextNode = child;
                    break;
                }
            }

            if (!nextNode) {
                auto* operations = node->concreteObject->operations;
                auto* concreteObject = operations->finddir(*node->concreteObject, pathComponent.str);
                if (!concreteObject)
                    return false;
                nextNode = VirtualFilesystem::instance().create_new_node(pathComponent.str, node, concreteObject);

                node->childNodes.push_back(nextNode);
            }
            VirtualFilesystem::instance().increment_node_reference_count(node);
            node->lock.release();
            nextNode->lock.acquire();
            if (!nextNode->concreteObject)
                return false;
            node = nextNode;
        }
        node->lock.release();
        deviceSpecific = node;
        return true;
    }

    void VirtualFilesystem::close(void*& deviceSpecific)
    {
        auto node = static_cast<Node*>(deviceSpecific);
        do {
            node->lock.acquire();
            auto* parent = node->parentNode;
            VirtualFilesystem::instance().decrement_node_reference_count(node);
            node->lock.release();
            node = parent;
        } while (node);
    }

    void VirtualFilesystem::increment_node_reference_count(Node* node)
    {
        if (node->refCount == 0) {
            // Remove from free list.
            if (node->freeListIterator.has_value()) {
                m_unusedNodes.erase(*node->freeListIterator);
                node->freeListIterator = Common::Nullopt;
            }
        }
        node->refCount++;
    }

    void VirtualFilesystem::decrement_node_reference_count(Node* node)
    {
        node->refCount--;
        if (node->refCount > 0)
            return;
        
        m_unusedNodes.push_back(node);
        auto last = --m_unusedNodes.end();
        node->freeListIterator = last;
    }

    VirtualFilesystem::Node* VirtualFilesystem::create_new_node(const char* name, Node* parentNode, ConcreteObject* concreteObject)
    {
        auto* node = new Node(name, parentNode);
        node->concreteObject = concreteObject;
        {
            LockAcquirer(node->concreteObject->lock);
            node->concreteObject->refCount++;
        }
        return node;
    }

    Path parse_path(const char* path)
    {
        Path ret;
        std::size_t front = 0;
        std::size_t back = 0;

        while (path[back]) {
            for (; path[back]; back++) {
                if (path[back] == PATH_SEPERATOR)
                    break;
            }
            ret.emplace_back();
            auto last = --ret.end();
            strncpy(last->str, path + front, back - front);
            last->str[back - front] = '\0';
            if (path[back]) {
                back++;
                front = back;
            }
        }
        return ret;
    }
}
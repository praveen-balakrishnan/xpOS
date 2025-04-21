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

#ifndef VFS_H
#define VFS_H

#include "Common/List.h"
#include "Common/Hashmap.h"
#include "Common/Optional.h"
#include "Common/String.h"
#include "Pipes/Pipe.h"

namespace Filesystem
{
struct ConcreteObject;

typedef std::size_t (*vfsdriver_read_callback)(ConcreteObject&, std::size_t, std::size_t, char*);
typedef std::size_t (*vfsdriver_write_callback)(ConcreteObject&, std::size_t, std::size_t, const char*);
typedef ConcreteObject* (*vfsdriver_finddir_callback)(const ConcreteObject&, char* name);
typedef std::size_t (*vfsdriver_fileinfo_callback)(const ConcreteObject&);

static constexpr int MAX_PATH_SIZE = 256;
static constexpr char PATH_SEPERATOR = '/';

/**
 * These operations are implemented by a concrete filesystem driver.
 * 
 * They are called by the virtual filesystem to handle user filesystem requests.
 */
struct DriverOperations
{
    vfsdriver_read_callback read;
    vfsdriver_write_callback write;
    vfsdriver_finddir_callback finddir;
    vfsdriver_fileinfo_callback fileinfo;
};

/**
 * A ConcreteObject represents anything in a concrete filesystem, such as files,
 * directories, symlinks etc.
 * 
 * 
 */
struct ConcreteObject
{
    friend class VirtualFilesystem;
    // Used by the driver to locate the filesystem-specific inode.
    uint64_t inodeNum;
    // Free for the driver to use.
    void* fs;

    const DriverOperations* operations;
    int refCount = 0;

    Spinlock lock;

    enum class Type {
        FILE,
        DIRECTORY,
        MOUNTPOINT
    } type;

    uint64_t creationTime = 0;
    uint64_t modificationTime = 0;
    uint64_t accessedTime = 0;
    uint64_t fileSize = 0;
private:
    ConcreteObject(const DriverOperations* operations)
        : operations(operations)
    {}
};

/**
 * Concrete filesystems are mounted in the virtual filesystem.
 * 
 * The virtual filesystem calls the concrete filesystem's driver for file operations.
 */ 
struct ConcreteFilesystemDescriptor
{
    const DriverOperations* driver;
    ConcreteObject* root;
    ConcreteFilesystemDescriptor(const DriverOperations* driver, ConcreteObject* root)
        : driver(driver)
        , root(root)
    {}
};

struct PathComponent
{
    char str[MAX_PATH_SIZE];
};

using Path = Common::List<PathComponent>;

Path parse_path(const char* path);

/**
 * The virtual filesystem acts as the abstracted interface for processes
 * to access files. Concrete filesystems can be mounted and its objects
 * accessed through the virtual filesystem.
 * 
 * It uses a hierarchical rooted tree to manage files, directories,
 * mountpoints etc. and this can be exposed to processes.
 */
class VirtualFilesystem
{
public:
    class Node;
    static VirtualFilesystem& instance()
    {
        static VirtualFilesystem instance;
        return instance;
    }
    VirtualFilesystem(const VirtualFilesystem&) = delete;
    VirtualFilesystem& operator=(const VirtualFilesystem&) = delete;

    // TODO: Add support for mountpoints, which should be relatively easy.
    int register_filesystem(ConcreteFilesystemDescriptor filesystem); 
    void unregister_filesystem(int mountNum);

    std::size_t filelen(Pipes::Pipe& pipe);
    ConcreteObject* create_concrete_object(const DriverOperations* operations)
    {
        return new ConcreteObject(operations);
    }
private:
    VirtualFilesystem();

    int m_fsAllocNum = 0;
    Node* m_rootNode;
    Common::Hashmap<Node*, ConcreteFilesystemDescriptor> m_mountedFilesystems;
    Common::List<Node*> m_unusedNodes;

    Pipes::DeviceOperations m_deviceOperations;

    Node* create_new_node(const char* name, Node* parentNode, ConcreteObject* concreteObject);
    void increment_node_reference_count(Node* node);
    void decrement_node_reference_count(Node* node);

    /**
     * These functions are called when a pipe is opened to the virtual filesystem.
     * They handle opening, closing, reading, and writing to files.
     */
    static bool open(void* with, int flags, void*& deviceSpecific);
    static void close(void*& deviceSpecific);
    static std::size_t read(std::size_t offset, std::size_t count, void* buf, void*& deviceSpecific);
    static std::size_t write(std::size_t offset, std::size_t count, const void* buf, void*& deviceSpecific);
    static xpOS::API::Pipes::PipeInfo info(void*& deviceSpecific);

public:
    /**
     * Filesystem objects are cached in a rooted tree structure using nodes.
     * 
     * This allows for fast navigation of the filesystem tree without having
     * to call into concrete filesystem drivers and potentially wait for slow
     * I/O accesses.
     */
    struct Node
    {
        int refCount = 0;
        Spinlock lock;

        char componentName[MAX_PATH_SIZE];
        ConcreteObject* concreteObject;

        Common::List<Node*> childNodes;
        Node* parentNode;
        
        using UnusedNodeList = Common::List<Node*>;
        Common::Optional<UnusedNodeList::Iterator> freeListIterator;

        Node(const char* componentName, Node* parentNode)
            : parentNode(parentNode)
        {
            strcpy(this->componentName, componentName);
        }
    };
};

struct taskfd
{
    uint64_t fd;
    uint64_t offset;
    VirtualFilesystem::Node* node;
};
}

#endif
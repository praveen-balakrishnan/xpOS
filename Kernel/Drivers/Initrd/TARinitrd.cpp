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

#include "Drivers/Initrd/TARinitrd.h"

namespace Filesystem {

// By resolving a path, we add directories to the directory map and add files to the file map.
void TarReader::resolve_path(char* path, uint64_t fileInodeNum, TarHeader* currentHeader)
{
    // We start with the root directory node.
    uint64_t workingDirectory = 0;
    while (path[0]) {
        bool isDirectory = false;
        uint64_t substringIndex = 0;
        // Check if the path contains a directory.
        for (char* pointer = path; *pointer != '\0'; pointer++) {
            if (*pointer == PATH_SEPERATOR) {
                isDirectory = true;
                substringIndex = pointer - path;
                break;
            }
        }
        if (isDirectory) {
            char directoryName[MAX_PATH_SIZE];
            // Get directory name
            strncpy(directoryName, path, substringIndex);
            directoryName[substringIndex] = '\0';
            strcpy(path, path + substringIndex + 1);

            auto workingDirectoryDescriptor = m_directoryMap.find(workingDirectory);
            auto it = workingDirectoryDescriptor->second.entries.begin();
            // Try and find directory in map.
            for (; it != workingDirectoryDescriptor->second.entries.end(); ++it) {
                if (!strcmp(it->filename, directoryName))
                    break;
            }
            // Add directory to directory map and to parent directory's entries.
            if (it == workingDirectoryDescriptor->second.entries.end()) {
                m_directoryMap.insert({++m_dirVirtualNodeNum, TarDirectoryDescriptor()});
                auto entry = TarDirectoryEntry();
                strcpy(entry.filename, directoryName);
                entry.inodeNum = m_dirVirtualNodeNum;
                m_directoryMap.find(workingDirectory)->second.entries.push_back(entry);
                workingDirectory = m_dirVirtualNodeNum;
            } else {
                workingDirectory = it->inodeNum;
            }
        } else {
            // Add file to directory and to file map.
            if (path[0] == '\0')
                return;
            auto entry = TarDirectoryEntry();
            entry.inodeNum = fileInodeNum;
            strcpy(entry.filename, path);
            m_directoryMap.find(workingDirectory)->second.entries.push_back(entry);
            m_fileMap.insert({fileInodeNum, currentHeader});
            path[0] = '\0';
        }
    }
}

void TarReader::load_initrd()
{
    auto* concreteObject = VirtualFilesystem::instance().create_concrete_object(&drops);
    concreteObject->inodeNum = 0;
    concreteObject->type = ConcreteObject::Type::DIRECTORY;

    m_mountNum = VirtualFilesystem::instance().register_filesystem(ConcreteFilesystemDescriptor(&drops, concreteObject));
    m_headerCount = 0;

    for (auto header = reinterpret_cast<TarHeader*>(INITRD_ADDRESS); *header->get_filename(); header = header->get_next_header())
        m_headerCount++;
    
    m_dirVirtualNodeNum = m_headerCount;

    // Insert the root directory descriptor.
    auto rootDescriptor = TarDirectoryDescriptor();
    m_directoryMap.insert({0, rootDescriptor});
    char substring[MAX_PATH_SIZE];

    int headerNo = 1;

    for (auto header = reinterpret_cast<TarHeader*>(INITRD_ADDRESS); *header->get_filename(); header = header->get_next_header()) {
        memcpy(substring, header->get_filename(), 100);
        resolve_path(substring, headerNo, header);
        headerNo++;
    }
}

uint64_t TarReader::vfsdriver_read(ConcreteObject& vnode, uint64_t offset, uint64_t size, char* buf)
{
    auto header = static_cast<TarHeader*>(vnode.fs);
    return instance().read_header(header, offset, size, (uint8_t*)buf);
}

ConcreteObject* TarReader::vfsdriver_finddir(const ConcreteObject& vnode, char* name)
{
    auto it = instance().m_directoryMap.find(vnode.inodeNum);
    auto& descriptor = it->second;
    for (auto& entry : descriptor.entries) {
        if (!strcmp(entry.filename, name))
            return instance().prepare_concrete_object(entry);
    }
    return nullptr;
}

uint64_t TarReader::vfsdriver_fileinfo(const ConcreteObject& vnode)
{
    TarHeader* header = (TarHeader*)vnode.fs;
    return header->get_size();
}

ConcreteObject* TarReader::prepare_concrete_object(TarDirectoryEntry entry)
{
    auto* concreteObject = VirtualFilesystem::instance().create_concrete_object(&drops);
    // If the inode number is less than the header count, then the inode number is the file header number.
    if ((entry.inodeNum <= m_headerCount) && entry.inodeNum) {
        concreteObject->type = ConcreteObject::Type::FILE;
        concreteObject->fs = m_fileMap.find(entry.inodeNum)->second;
    } else {
        concreteObject->type = ConcreteObject::Type::DIRECTORY;
    }
    concreteObject->inodeNum = entry.inodeNum;
    return concreteObject;
}

uint64_t TarReader::read_header(TarHeader* header, uint64_t offset, uint64_t size, uint8_t* buf)
{
    auto content = reinterpret_cast<uint8_t*>(header) + TAR_ALIGN + offset;
    memcpy(buf, content, size);
    return size;
}

}
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
#ifndef TARINITRD_H
#define TARINITRD_H

#include "Common/Hashmap.h"
#include "Common/List.h"
#include "Common/String.h"
#include "Filesystem/VFS.h"

namespace Filesystem {

class TarReader
{
private:
    class TarHeader
    {
    private:
        char m_filename[100];
        char m_mode[8];
        char m_uid[8];
        char m_gid[8];
        char m_size[12];
        char m_mtime[12];
        char m_chksum[8];
        char m_typeflag[1];
        char m_reserved[355];

    public:
        char* get_filename()
        {
            return m_filename;
        }

        void set_filename(char* filename)
        {
            strcpy(m_filename, filename);
        }

        uint64_t get_size()
        {
            uint64_t size = 0;
            unsigned int i;
            uint64_t count = 1;

            // The size is represented in octal, so we must convert it to decimal.
            for (i = 11; i > 0; i--, count *= 8)
                size += ((m_size[i - 1] - '0') * count);
        
            return size; 
        }

        TarHeader* get_next_header()
        {
            auto header = this;
            header++;
            // Each header is aligned to 512 bytes, so we need to round up.
            auto pointer = reinterpret_cast<uint8_t*>(header);
            pointer += BYTE_ALIGN_UP(get_size(), TAR_ALIGN);
            return reinterpret_cast<TarHeader*>(pointer);
        }
    } __attribute__((packed));

    struct TarDirectoryEntry
    {
        char filename[256];
        uint64_t inodeNum;
    };

    struct TarDirectoryDescriptor
    {
        Common::List<TarDirectoryEntry> entries;
    };

public:
    void load_initrd();

    // Required operations for the concrete filesystem to be mountable
    static uint64_t vfsdriver_read(ConcreteObject&, uint64_t, uint64_t, char*);
    static ConcreteObject* vfsdriver_finddir(const ConcreteObject&, char* name);
    static uint64_t vfsdriver_fileinfo(const ConcreteObject&);
    static constexpr DriverOperations drops = {
        .read = vfsdriver_read,
        .finddir = vfsdriver_finddir,
        .fileinfo = vfsdriver_fileinfo
    };

    static TarReader& instance()
    {
        static TarReader instance;
        return instance;
    }
    TarReader(TarReader const&) = delete;
    TarReader& operator=(TarReader const&) = delete;
private:
    static constexpr uint64_t INITRD_ADDRESS = 0xFFFFFFFFA0000000;
    static constexpr uint64_t TAR_ALIGN = 0x200;
    TarReader() {}
    uint64_t m_mountNum;
    uint64_t m_headerCount;
    Common::Hashmap<uint64_t, TarDirectoryDescriptor> m_directoryMap;
    Common::Hashmap<uint64_t, TarHeader*> m_fileMap;
    uint64_t m_dirVirtualNodeNum;
    void resolve_path(char* substr, uint64_t fileInodeNum, TarHeader* currentHeader);
    ConcreteObject* prepare_concrete_object(TarDirectoryEntry entry);
    uint64_t read_header(TarHeader* header, uint64_t offset, uint64_t size, uint8_t* buf);
};

}

#endif
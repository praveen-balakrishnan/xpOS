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

#include <cstring>
#include <optional>
#include "WAV.h"

namespace Audio::WAV
{
    std::optional<const ChunkHeader*> find_chunk(const ChunkHeader* header, std::size_t length, const char* str)
    {
        while (strncmp(header->chunkId, str, 4)) {
            if (length < sizeof(ChunkHeader) + header->chunkSize)
                return {};
            length -= sizeof(ChunkHeader) + header->chunkSize;
            auto* ptr = reinterpret_cast<const uint8_t*>(header);
            ptr += sizeof(ChunkHeader);
            ptr += header->chunkSize;
            header = reinterpret_cast<const ChunkHeader*>(ptr);
        }
        return header;
    }

    std::optional<Metadata> read_metadata(const uint8_t* wavData, std::size_t length)
    {
        auto remaining = length;
        if (remaining < sizeof(RIFFHeader))
            return {};

        auto* riffHeader = reinterpret_cast<const RIFFHeader*>(wavData);
        if (strncmp(riffHeader->magic, "RIFF", 4))
            return {};
        if (strncmp(riffHeader->type, "WAVE", 4))
            return {};
        
        remaining -= sizeof(RIFFHeader);
        if (remaining < sizeof(ChunkHeader))
            return {};

        auto* chunkHeader = reinterpret_cast<const ChunkHeader*>(riffHeader + 1);
        
        std::optional<const ChunkHeader*> fmtChunkOpt = find_chunk(chunkHeader, remaining, "fmt ");
        if (!fmtChunkOpt.has_value())
            return {};

        auto* fmtChunk = reinterpret_cast<const BasicFMTChunk*>(*fmtChunkOpt + 1);

        Metadata m;
        m.format = static_cast<FormatCode>(fmtChunk->formatCode);
        m.channelCount = fmtChunk->channelCount;
        m.sampleRate = fmtChunk->sampleRate;
        m.bitsPerSample = fmtChunk->bitsPerSample;

        auto dataChunkOpt = find_chunk(chunkHeader, remaining, "data");
        if (!dataChunkOpt.has_value())
            return {};
        
        m.dataOffset = static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(*dataChunkOpt + 1) - wavData);

        return m;
    }
}

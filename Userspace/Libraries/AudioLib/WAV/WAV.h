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

#ifndef SOUND_WAV_H
#define SOUND_WAV_H

#include <cstdint>
#include <optional>
#include "API/Endian.h"

namespace Audio::WAV
{
    enum FormatCode : uint16_t
    {
        PCM = 0x0001,
        IEEE_FLOAT = 0x0003,
        ALAW = 0x0006,
        MULAW = 0x0007,
        EXT = 0xFFFE
    };

    struct Metadata
    {
        FormatCode format;
        uint16_t channelCount;
        uint32_t sampleRate;
        uint8_t bitsPerSample;
        uint32_t dataOffset;
    };

    struct [[gnu::packed]] RIFFHeader
    {
        char magic[4];
        uint32_t size;
        char type[4];
    };
    
    struct [[gnu::packed]] ChunkHeader
    {
        char chunkId[4];
        uint32_t chunkSize;
    };

    struct [[gnu::packed]] BasicFMTChunk
    {
        uint16_t formatCode;
        uint16_t channelCount;
        uint32_t sampleRate;
        uint32_t dataRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
        uint16_t extSize;
    };

    struct [[gnu::packed]] FMTChunkExtension
    {
        uint16_t validBits;
        uint32_t channelMask;
        uint64_t subformatLow;
        uint64_t subformatHigh;
    };

    std::optional<Metadata> read_metadata(const uint8_t* wavData, std::size_t length);
}

#endif

#ifndef SERIALISED_DATA_H
#define SERIALISED_DATA_H

//#include "common/Vector.h"
#include "Archive.h"
#include <cstdint>
#include <string>
#include <list>
#include <vector>

namespace Serialisation
{

struct SerialisedData
{
public:
    SerialisedData()
    {
        m_rawBytes.reserve(1024);
    }

    friend auto& operator>>(ArchivableTo<SerialisedData> auto& data, SerialisedData& self)
    {
        data.define_archivable(self);
        return self;
    }

    template<typename... Args>
    void operator()(Args&... args)
    {
        ([&]
        {
            encode(args);
        } (), ...);
    }

    template <typename T>
    void encode(const T& encodable);

    void push(const uint8_t* values, std::size_t count)
    {
        for (std::size_t i = 0; i < count; i++) 
            m_rawBytes.push_back(values[i]);
    }

    const uint8_t* bytes() 
    {
        return m_rawBytes.data();
    }

    auto length()
    {
        return m_rawBytes.size();
    }

private:
    std::vector<uint8_t> m_rawBytes;
};

void encode(const std::integral auto& encodable, SerialisedData& msg)
{
    msg.push(reinterpret_cast<const uint8_t*>(&encodable), sizeof(encodable));
}

void encode(const std::floating_point auto& encodable, SerialisedData& msg)
{
    msg.push(reinterpret_cast<const uint8_t*>(&encodable), sizeof(encodable));
}

void encode(const std::string& encodable, SerialisedData& msg);

template<typename T>
void encode(const std::vector<T>& encodable, SerialisedData& msg)
{
    std::size_t sz = encodable.size();
    msg.push(reinterpret_cast<const uint8_t*>(&sz), sizeof(sz));
    for (auto const& item : encodable)
        encode(item, msg);
}


void encode(const auto& encodable, SerialisedData& msg)
{
    encodable.define_archivable(msg);
}

template <typename T>
void SerialisedData::encode(const T& encodable)
{
    Serialisation::encode(encodable, *this);
}

}

#endif
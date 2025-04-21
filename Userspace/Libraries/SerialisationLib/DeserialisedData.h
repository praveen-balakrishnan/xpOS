#ifndef DESERIALISED_DATA_H
#define DESERIALISED_DATA_H

#include "Archive.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <list>
#include <vector>


namespace Serialisation
{

struct DeserialisedData
{
public:
    DeserialisedData(
        const uint8_t* buf,
        std::size_t count
    )
    {
        //m_rawBytes = reinterpret_cast<uint8_t*>(kmalloc(count, 0));
        m_rawBytes.reserve(count);
        m_currentOffset = 0;
        std::memcpy(m_rawBytes.data(), buf, count);
    }

    friend auto& operator<<(ArchivableTo<DeserialisedData> auto& data, DeserialisedData& self)
    {
        data.define_archivable(self);
        return self;
    }

    template<typename... Args>
    void operator()(Args&... args)
    {
        ([&]
        {
            decode(args);
        } (), ...);
    }

    template <typename T>
    void decode(T& decodable);

    const uint8_t* pop(std::size_t count)
    {
        m_currentOffset += count;
        return m_rawBytes.data() + m_currentOffset - count;
    }

private:
    std::vector<uint8_t> m_rawBytes;
    int m_currentOffset;
};

void decode(std::integral auto& decodable, DeserialisedData& msg)
{
    decodable = *reinterpret_cast<const std::remove_reference_t<std::remove_cv_t<decltype(decodable)>>*>(msg.pop(sizeof(decodable)));
}

void decode(std::floating_point auto& decodable, DeserialisedData& msg)
{
    decodable = *reinterpret_cast<const std::remove_reference_t<std::remove_cv_t<decltype(decodable)>>*>(msg.pop(sizeof(decodable)));
}

void decode(std::string& decodable, DeserialisedData& msg);

template <typename T>
void decode(std::vector<T>& decodable, DeserialisedData& msg)
{
    std::size_t sz;
    decode(sz, msg);
    decodable.clear();
    decodable.reserve(sz);
    for (std::size_t i = 0; i < sz; i++) {
        /// FIXME: We do not want to require that T is default constructible.
        T t;
        decode(t, msg);
        decodable.push_back(t);
    }
}

template <typename T>
void decode(std::list<T>& decodable, DeserialisedData& msg)
{
    std::size_t sz;
    decode(sz, msg);
    decodable.clear();
    for (std::size_t i = 0; i < sz; i++) {
        /// FIXME: We do not want to require that T is default constructible.
        T t;
        decode(t, msg);
        decodable.push_back(t);
    }
}

void decode(auto& decodable, DeserialisedData& msg)
{
    decodable.define_archivable(msg);
}

template <typename T>
void DeserialisedData::decode(T& decodable)
{
    Serialisation::decode(decodable, *this);
}

}

#endif
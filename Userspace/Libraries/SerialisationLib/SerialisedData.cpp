#include "SerialisedData.h"

#include <string>

namespace Serialisation
{

void encode(const std::string& encodable, SerialisedData& msg)
{
    std::size_t sz = encodable.size();
    msg.push(reinterpret_cast<const uint8_t*>(&sz), sizeof(sz));
    for (char c : encodable)
        encode(c, msg);
}

}
#include "DeserialisedData.h"

namespace Serialisation
{

void decode(std::string& decodable, DeserialisedData& msg)
{
    std::size_t sz;
    decode(sz, msg);
    decodable.clear();
    for (std::size_t i = 0; i < sz; i++) {
        /// FIXME: We do not want to require that T is default constructible.
        char c;
        decode(c, msg);
        decodable += c;
    }
}

}
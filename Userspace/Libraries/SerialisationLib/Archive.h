#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <concepts>

namespace Serialisation
{

template <typename D, typename Archive>
concept ArchivableTo = requires(D d, Archive ar)
{
    d.define_archivable(ar);
};

}

#endif
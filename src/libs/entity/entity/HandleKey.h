#pragma once

#include <cstddef>


namespace ad {
namespace ent {


class HandleKey
{
public:
    HandleKey() = default;

    HandleKey(std::size_t aIndex) :
        mIndex{aIndex}
    {}

    bool operator==(const HandleKey & aRhs) const = default;

    /*implicit: for array access*/ operator std::size_t () const
    { return mIndex; }

    HandleKey operator++(int /*postfix*/)
    {
        return HandleKey{mIndex++};
    }

private:
    std::size_t mIndex{0};
};


using ArchetypeKey = HandleKey;


} // namespace ent
} // namespace ad

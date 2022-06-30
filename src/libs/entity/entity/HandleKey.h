#pragma once

#include <cstddef>


namespace ad {
namespace ent {


class HandleKey
{
public:
    HandleKey() = default;

    operator std::size_t () const
    { return mIndex; }

    HandleKey operator++(int /*postfix*/)
    {
        return HandleKey{mIndex++};
    }

private:
    HandleKey(std::size_t aIndex) :
        mIndex{aIndex}
    {}

    std::size_t mIndex{0};
};


} // namespace ent
} // namespace ad

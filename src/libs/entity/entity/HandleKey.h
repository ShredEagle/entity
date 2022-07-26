#pragma once

#include <cstddef>


namespace ad {
namespace ent {


/// \note The class argument is intended to have distinct types depending of what is handled.
template <class>
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


} // namespace ent
} // namespace ad

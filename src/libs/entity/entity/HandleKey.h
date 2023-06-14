#pragma once

#include <cstddef>


namespace ad {
namespace ent {


/// \note The class argument is intended to have distinct types depending of what is handled.
template <class>
class HandleKey
{
public:
    using Underlying_t = std::size_t;

    static constexpr auto gInvalidKey = std::numeric_limits<Underlying_t>::max();

    HandleKey() = delete;

    constexpr HandleKey(Underlying_t aIndex) :
        mIndex{aIndex}
    {}

    constexpr bool operator==(const HandleKey & aRhs) const = default;

    /*implicit: for array access*/ constexpr operator Underlying_t () const
    { return mIndex; }

    constexpr HandleKey operator++(int /*postfix*/)
    {
        return HandleKey{mIndex++};
    }

private:
    Underlying_t mIndex;
};


} // namespace ent
} // namespace ad

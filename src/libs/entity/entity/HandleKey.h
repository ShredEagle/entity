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

    // Note that the functionality of "Invalid Handle<Entity>" does not need a special HandleKey value
    // since it simply use the value to get an invalid record in a dedicated EntityManager (and it could be associated to a normal key).
    // But this might make debugging easier to have a special value.
    static constexpr auto gInvalidKey = std::numeric_limits<Underlying_t>::max();

    HandleKey() = delete;

    constexpr HandleKey(Underlying_t aIndex) :
        mIndex{aIndex}
    {}
    static constexpr HandleKey MakeInvalid()
    {
        HandleKey invalid{0};
        invalid.mIndex = gInvalidKey;
        return invalid;
    }


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

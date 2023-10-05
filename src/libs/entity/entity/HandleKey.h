#pragma once

#include <cassert>
#include <cstddef>


namespace ad {
namespace ent {


/// \note The class template type argument is intended to have distinct types depending of what is handled.
/// \brief Abstraction for the key value of an Handle, it uses bit manipulation to store several logic values in an integer. 
/// Currently stores an index (to access the entity in a registry),
/// and a generation (to ensure the pointed Entity matches the HandleKey, and is not a reuse of the same index).
/// \see: https://ajmmertens.medium.com/doing-a-lot-with-a-little-ecs-identifiers-25a72bd2647
template <class>
class HandleKey
{
public:
    // TODO use sized type
    using Underlying_t = std::size_t;

private:
    static_assert(sizeof(Underlying_t) == 8, "The underlying type must be 64 bits.");

    // Note that the functionality of "Invalid Handle<Entity>" does not need a special HandleKey value
    // since it simply use the value to get an invalid record in a dedicated EntityManager (and it could be associated to a normal key).
    // But this might make debugging easier to have a special value.
    static constexpr auto gMaxValue = std::numeric_limits<Underlying_t>::max();

    // The N high order bits are used for the generation.
    static constexpr auto gGenerationBits = 24;
    static constexpr auto gGenerationShift = (64 - gGenerationBits);

    static constexpr Underlying_t gGenerationMask = std::numeric_limits<Underlying_t>::max() << gGenerationShift;
    static constexpr Underlying_t gIndexMask = ~gGenerationMask;

public:
    HandleKey() = delete;

    static constexpr HandleKey MakeFirst()
    {
        return HandleKey{0};
    }

    /// \brief This HandleKey is not invalid, but has all bits to 1.
    /// This is intended for the invalid default constructed Handle, to be easily spotted in debug.
    static constexpr HandleKey MakeLatest()
    {
        return HandleKey{gMaxValue};
    }

    /// \brief Make the handle key with provided index, and first generation.
    static constexpr HandleKey MakeIndex(Underlying_t aIndex)
    {
        assert((aIndex & gGenerationMask) == 0);
        return HandleKey{aIndex & gIndexMask};
    }

    constexpr bool operator==(const HandleKey & aRhs) const = default;

    //TODO: too dangerous !!! because things like "this | 23" removes the generation
    // data from the result (meaning that type coercion might remove the generation from the handle key)
    /// \brief Return the index value (discarding the generation).
    /*implicit: for array access*/ constexpr operator Underlying_t () const
    { return mGenerationAndIndex & gIndexMask; }

    /// \brief Increment the index value (keeping the same generation).
    constexpr HandleKey operator++(int /*postfix*/)
    {
        // Assign the increment index part 
        // (mask ensures that wrapping over the index does not bleed on generation)
        HandleKey incremented{mGenerationAndIndex++ & gIndexMask};
        // Copy over the generation
        incremented.mGenerationAndIndex |= mGenerationAndIndex & gGenerationMask;
        return incremented;
    }

    // TODO this being const is a massive code smell.
    // Adressing it require to either store the generation in the EntityRecord (which is non-const in the map, contrary to the key)
    // or replacing the map altogether, which is probably the way to go (e.g. SparseSet).
    /// \brief Increment the generation.
    constexpr const HandleKey & advanceGeneration() const
    {
        auto newGeneration = ((mGenerationAndIndex >> gGenerationShift) + 1) << gGenerationShift;
        mGenerationAndIndex = newGeneration | (mGenerationAndIndex & gIndexMask);
        return *this;
    }

    /// \brief Return true if the HandleKey generation is the last generation 
    /// Advancing the generation after the last wraps around,
    /// with an associated risk of collision.
    constexpr bool isLastGeneration() const
    {
        return (mGenerationAndIndex >> gGenerationShift) == gGenerationMask;
    }

    /// \brief Compare the index only, disregard generation.
    struct LessIndex
    {
        bool operator()(const HandleKey & aLhs, const HandleKey & aRhs) const
        {
            return (Underlying_t)aLhs < (Underlying_t)aRhs;
        }
    };

private:
    constexpr HandleKey(Underlying_t aGenerationAndIndex) :
        mGenerationAndIndex{aGenerationAndIndex}
    {}

    // We need to advance generation on HandleKey<Entity> used as map key (so only returned as const ref)
    mutable Underlying_t mGenerationAndIndex;
};


} // namespace ent
} // namespace ad

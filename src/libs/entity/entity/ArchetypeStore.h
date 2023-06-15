#pragma once


#include "Archetype.h"
#include "Entity.h"
#include "entity/HandleKey.h"

#include <map>
#include <memory>
#include <vector>


namespace ad {
namespace ent {


class ArchetypeStore
{
public:
    std::pair<Archetype &, HandleKey<Archetype>> getEmptyArchetype();

    ArchetypeStore & operator=(const ArchetypeStore & aRhs);

    Archetype & get(HandleKey<Archetype> aKey);
    const Archetype & get(HandleKey<Archetype> aKey) const;

    HandleKey<Archetype> getKey(TypeSet aTypeSet);

    auto beginMap() const
    { return mTypeSetToArchetype.begin(); }
    auto endMap() const
    { return mTypeSetToArchetype.end(); }

    auto size() const
    { return mHandleToArchetype.size(); }

    /// \return The HandleKey to the archetype matching `aTargetTypeSet`,
    /// and true if it was inserted, false if it was already present.
    template <class F_maker>
    std::pair<HandleKey<Archetype>, bool> makeIfAbsent(const TypeSet & aTargetTypeSet,
                                                       F_maker aMakeCallback);

private:
    static std::vector<std::unique_ptr<Archetype>> getInitialVector()
    { 
        std::vector<std::unique_ptr<Archetype>> result;
        result.push_back(std::make_unique<Archetype>());
        return result;
    };

    inline static const TypeSet gEmptyTypeSet{};
    // Has to be zero (first index in the vector)
    static constexpr HandleKey<Archetype> gEmptyTypeSetArchetypeHandle{0};

    // Initially, we stored the archetype by value in the vector
    // Yet on reallocation, this would invalidate all reference to the archetype
    // (notably, to its vector of handles in Query::each())
    std::vector<std::unique_ptr<Archetype>> mHandleToArchetype{getInitialVector()};
    std::map<TypeSet, HandleKey<Archetype>> mTypeSetToArchetype{
        {
            gEmptyTypeSet,
            gEmptyTypeSetArchetypeHandle, 
        }
    };
};


inline std::pair<Archetype &, HandleKey<Archetype>> ArchetypeStore::getEmptyArchetype()
{
    return {
        *mHandleToArchetype[gEmptyTypeSetArchetypeHandle],
        gEmptyTypeSetArchetypeHandle,
    };
}


inline Archetype & ArchetypeStore::get(HandleKey<Archetype> aKey)
{
    return *mHandleToArchetype.at(aKey);
}

inline const Archetype & ArchetypeStore::get(HandleKey<Archetype> aKey) const
{
    return *mHandleToArchetype.at(aKey);
}


inline HandleKey<Archetype> ArchetypeStore::getKey(TypeSet aTypeSet)
{
    return mTypeSetToArchetype.at(aTypeSet);
}


template <class F_maker>
std::pair<HandleKey<Archetype>, bool> ArchetypeStore::makeIfAbsent(const TypeSet & aTargetTypeSet,
                                                                   F_maker aMakeCallback)
{
    if (auto found = mTypeSetToArchetype.find(aTargetTypeSet);
        found != mTypeSetToArchetype.end())
    {
        return {found->second, false};
    }
    else
    {
        std::size_t insertedPosition = mHandleToArchetype.size();
        mHandleToArchetype.push_back(aMakeCallback());
        HandleKey<Archetype> inserted = mTypeSetToArchetype.emplace(
            aTargetTypeSet,
            HandleKey<Archetype>{insertedPosition})
            .first->second;
        return {inserted, true};
    }
}


} // namespace ent
} // namespace ad

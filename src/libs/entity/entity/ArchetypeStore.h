#pragma once


#include "Archetype.h"
#include "Entity.h"
#include "entity/HandleKey.h"

#include <map>
#include <vector>


namespace ad {
namespace ent {


class ArchetypeStore
{
public:
    std::pair<Archetype &, HandleKey<Archetype>> getEmptyArchetype();

    Archetype & get(HandleKey<Archetype> aKey);
    const Archetype & get(HandleKey<Archetype> aKey) const;

    HandleKey<Archetype> getKey(TypeSet aTypeSet);

    auto beginMap() const
    { return mTypeSetToArchetype.begin(); }
    auto endMap() const
    { return mTypeSetToArchetype.end(); }

    auto size() const
    { return mHandleToArchetype.size(); }

    template <class F_maker>
    std::pair<HandleKey<Archetype>, bool> makeIfAbsent(const TypeSet & aTargetTypeSet,
                                               F_maker aMakeCallback);

private:
    inline static const TypeSet gEmptyTypeSet{};

    std::vector<Archetype> mHandleToArchetype{Archetype{}};
    std::map<TypeSet, HandleKey<Archetype>> mTypeSetToArchetype{
        {
            gEmptyTypeSet,
            {}
        }
    };
};


inline std::pair<Archetype &, HandleKey<Archetype>> ArchetypeStore::getEmptyArchetype()
{
    return {
        mHandleToArchetype[0],
        {},
    };
}


inline Archetype & ArchetypeStore::get(HandleKey<Archetype> aKey)
{
    return mHandleToArchetype.at(aKey);
}

inline const Archetype & ArchetypeStore::get(HandleKey<Archetype> aKey) const
{
    return mHandleToArchetype.at(aKey);
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

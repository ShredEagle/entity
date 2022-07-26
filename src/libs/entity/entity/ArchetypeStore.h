#pragma once


#include "Archetype.h"
#include "Entity.h"

#include <map>
#include <vector>


namespace ad {
namespace ent {


class ArchetypeStore
{
public:
    std::pair<Archetype &, ArchetypeKey> getEmptyArchetype();

    Archetype & get(ArchetypeKey aKey);
    const Archetype & get(ArchetypeKey aKey) const;

    ArchetypeKey getKey(TypeSet aTypeSet);

    auto beginMap() const
    { return mTypeSetToArchetype.begin(); }
    auto endMap() const
    { return mTypeSetToArchetype.end(); }

    auto size() const
    { return mHandleToArchetype.size(); }

    template <class F_maker>
    std::pair<ArchetypeKey, bool> makeIfAbsent(const TypeSet & aTargetTypeSet,
                                               F_maker aMakeCallback);

private:
    inline static const TypeSet gEmptyTypeSet{};

    std::vector<Archetype> mHandleToArchetype{Archetype{}};
    std::map<TypeSet, HandleKey> mTypeSetToArchetype{
        {
            gEmptyTypeSet,
            {}
        }
    };
};


inline std::pair<Archetype &, ArchetypeKey> ArchetypeStore::getEmptyArchetype()
{
    return {
        mHandleToArchetype[0],
        {},
    };
}


inline Archetype & ArchetypeStore::get(ArchetypeKey aKey)
{
    return mHandleToArchetype.at(aKey);
}

inline const Archetype & ArchetypeStore::get(ArchetypeKey aKey) const
{
    return mHandleToArchetype.at(aKey);
}


inline ArchetypeKey ArchetypeStore::getKey(TypeSet aTypeSet)
{
    return mTypeSetToArchetype.at(aTypeSet);
}


template <class F_maker>
std::pair<ArchetypeKey, bool> ArchetypeStore::makeIfAbsent(const TypeSet & aTargetTypeSet,
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
        ArchetypeKey inserted = mTypeSetToArchetype.emplace(
            aTargetTypeSet,
            ArchetypeKey{insertedPosition})
            .first->second;
        return {inserted, true};
    }
}


} // namespace ent
} // namespace ad

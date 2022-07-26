#pragma once


#include "Archetype.h"
#include "Entity.h"

#include <map>
#include <vector>


namespace ad {
namespace ent {


// TODO If this remains with handle only, remove and replace with handle.
struct ArchetypeRecord
{
    Handle<Archetype> mHandle;
};


class ArchetypeStore
{
    // TODO should not be required... but currently is because of makeArchetypeIfAbsent()
    friend class EntityManager;

public:
    std::pair<Archetype &, Handle<Archetype>> getEmptyArchetype();

    Archetype & get(Handle<Archetype> aHandle);
    const Archetype & get(Handle<Archetype> aHandle) const;

    Handle<Archetype> getHandle(TypeSet aTypeSet);

    auto beginMap() const
    { return mArchetypes.begin(); }
    auto endMap() const
    { return mArchetypes.end(); }

    auto size() const
    { return mHandleToArchetype.size(); }

    // TODO
    //std::pair<Handle<Archetype>, bool> makeIfAbsent(const TypeSet & aTargetTypeSet,
    //                                                F_maker aMakeCallback);

private:
    inline static const TypeSet gEmptyTypeSet{};

    std::vector<Archetype> mHandleToArchetype{Archetype{}};
    std::map<TypeSet, ArchetypeRecord> mArchetypes{
        {
            gEmptyTypeSet,
            {Handle<Archetype>{0}}
        }
    };
};


inline std::pair<Archetype &, Handle<Archetype>> ArchetypeStore::getEmptyArchetype()
{
    return {
        mHandleToArchetype[0],
        Handle<Archetype>{0},
    };
}


inline Archetype & ArchetypeStore::get(Handle<Archetype> aHandle)
{
    return mHandleToArchetype.at(aHandle.mKey);
}

inline const Archetype & ArchetypeStore::get(Handle<Archetype> aHandle) const
{
    return mHandleToArchetype.at(aHandle.mKey);
}


inline Handle<Archetype> ArchetypeStore::getHandle(TypeSet aTypeSet)
{
    return mArchetypes.at(aTypeSet).mHandle;
}


} // namespace ent
} // namespace ad

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
public:
    std::pair<Archetype &, Handle<Archetype>> getEmptyArchetype();

    Archetype & get(Handle<Archetype> aHandle);
    const Archetype & get(Handle<Archetype> aHandle) const;

    Handle<Archetype> getHandle(TypeSet aTypeSet);

    auto beginMap() const
    { return mTypeSetToArchetype.begin(); }
    auto endMap() const
    { return mTypeSetToArchetype.end(); }

    auto size() const
    { return mHandleToArchetype.size(); }

    template <class F_maker>
    std::pair<Handle<Archetype>, bool> makeIfAbsent(const TypeSet & aTargetTypeSet,
                                                    F_maker aMakeCallback);

private:
    inline static const TypeSet gEmptyTypeSet{};

    std::vector<Archetype> mHandleToArchetype{Archetype{}};
    std::map<TypeSet, ArchetypeRecord> mTypeSetToArchetype{
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
    return mTypeSetToArchetype.at(aTypeSet).mHandle;
}


template <class F_maker>
std::pair<Handle<Archetype>, bool> ArchetypeStore::makeIfAbsent(const TypeSet & aTargetTypeSet,
                                                                F_maker aMakeCallback)
{
    if (auto found = mTypeSetToArchetype.find(aTargetTypeSet);
        found != mTypeSetToArchetype.end())
    {
        return {found->second.mHandle, false};
    }
    else
    {
        std::size_t insertedPosition = mHandleToArchetype.size();
        mHandleToArchetype.push_back(aMakeCallback());
        ArchetypeRecord inserted = mTypeSetToArchetype.emplace(
            aTargetTypeSet,
            ArchetypeRecord{
                Handle<Archetype>{insertedPosition}
            })
            .first->second;
        return {inserted.mHandle, true};
    }
}


} // namespace ent
} // namespace ad

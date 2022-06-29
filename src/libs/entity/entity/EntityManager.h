#pragma once

#include "Archetype.h"
#include "Entity.h"

#include <algorithm>
#include <map>

#include <cstddef>


namespace ad {
namespace ent {


// TODO implement handle reuse
class EntityManager
{
    friend class Handle<Entity>;
    template <class...>
    friend class Query;

    // To be implemented by the test application, allowing to access private parts.
    template <class>
    friend class Inspector;

public:
    //EntityManager();

    /// \warning Thread unsafe!
    Handle<Entity> addEntity();

    std::size_t countLiveEntities() const;

private:
    Archetype & getArchetype(const TypeSet & aTypeSet);

    template <class T_component>
    Archetype & extendArchetype(const Archetype & aArchetype);

    HandleKey mNextHandle;
    std::map<HandleKey, EntityRecord> mHandleMap;
    std::map<TypeSet, Archetype> mArchetypes;
    inline static const TypeSet gEmptyTypeSet{};
};


//
// Implementations
//
// NOTE: In this file because it needs EntityManager definition.
template <class T_component>
void Handle<Entity>::add(T_component aComponent)
{
    EntityRecord initialRecord = record();

    Archetype & targetArchetype =
        mManager.extendArchetype<T_component>(*initialRecord.mArchetype);
    initialRecord.mArchetype->move(initialRecord.mIndex, targetArchetype);
    EntityIndex newIndex = targetArchetype.push(std::move(aComponent));

    EntityRecord newRecord{
        .mArchetype = &targetArchetype,
        .mIndex = newIndex,
    };
    updateRecord(newRecord);
}


inline Handle<Entity> EntityManager::addEntity()
{
    auto & archetype = mArchetypes[gEmptyTypeSet];
    mHandleMap[mNextHandle] = EntityRecord{
        &archetype,
        0, // no data anyway
    };

    return Handle<Entity>{
        mNextHandle++,
        *this
    };
}


template <class T_component>
Archetype & EntityManager::extendArchetype(const Archetype & aArchetype)
{
    TypeSet targetTypeSet{aArchetype.getTypeSet()};
    targetTypeSet.insert(getId<T_component>());

    if (auto found = mArchetypes.find(targetTypeSet);
        found != mArchetypes.end())
    {
        return found->second;
    }
    else
    {
        return mArchetypes.emplace(targetTypeSet, aArchetype.makeAdded<T_component>())
                .first->second;
    }
}


} // namespace ent
} // namespace ad

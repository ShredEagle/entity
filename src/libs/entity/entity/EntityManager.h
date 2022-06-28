#pragma once

#include "Archetype.h"

#include <algorithm>
#include <map>

#include <cstddef>


namespace ad {
namespace ent {


class HandleKey
{
public:
    HandleKey() = default;

    operator std::size_t () const
    { return mIndex; }

    HandleKey operator++(int /*postfix*/)
    {
        return HandleKey{mIndex++};
    }

private:
    HandleKey(std::size_t aIndex) :
        mIndex{aIndex}
    {}

    std::size_t mIndex{0};
};


struct EntityRecord
{
    Archetype * mArchetype;
    EntityIndex mIndex;
};


class EntityManager;


class Entity
{
    friend class EntityManager;

public:
    /// \warning Thread unsafe!
    template <class T_component>
    Entity & add(T_component aComponent);

    template <class T_component>
    bool has();

    template <class T_component>
    T_component & get();

private:
    Entity(HandleKey aKey, EntityManager & aManager) :
        mKey{aKey},
        mManager{aManager}
    {}

    EntityRecord record() const;

    void updateRecord(EntityRecord aNewRecord);

    HandleKey mKey;
    EntityManager & mManager;
};


// TODO implement handle reuse
class EntityManager
{
    friend class Entity;
    template <class...>
    friend class Query;

    // To be implemented by the test application, allowing to access private parts.
    template <class>
    friend class Inspector;

public:
    //EntityManager();

    /// \warning Thread unsafe!
    Entity addEntity();

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
template <class T_component>
Entity & Entity::add(T_component aComponent)
{
    EntityRecord initialRecord = record();

    Archetype & targetArchetype = mManager.extendArchetype<T_component>(*initialRecord.mArchetype);
    initialRecord.mArchetype->move(initialRecord.mIndex, targetArchetype);
    EntityIndex newIndex = targetArchetype.push(std::move(aComponent));

    EntityRecord newRecord{
        .mArchetype = &targetArchetype,
        .mIndex = newIndex,
    };
    updateRecord(newRecord);

    return *this;
}


template <class T_component>
bool Entity::has()
{
    return record().mArchetype->has<T_component>();
}


template <class T_component>
T_component & Entity::get()
{
    return record().mArchetype->get<T_component>(record().mIndex);
}


inline Entity EntityManager::addEntity()
{
    auto & archetype = mArchetypes[gEmptyTypeSet];
    mHandleMap[mNextHandle] = EntityRecord{
        &archetype,
        0, // no data anyway
    };

    return Entity{
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

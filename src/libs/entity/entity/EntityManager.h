#pragma once

#include "Archetype.h"
#include "Entity.h"
#include "entity/detail/QueryBackend.h"

#include <algorithm>
#include <deque>
#include <map>

#include <cstddef>


namespace ad {
namespace ent {


// TODO implement reuse of handle from the free list
class EntityManager
{
    friend class Archetype;
    friend class Handle<Entity>;
    template <class...>
    friend class Query;

    // To be implemented by the test application, allowing to access private parts.
    template <class>
    friend class Inspector;

public:
    //EntityManager();

    /// \warning Thread unsafe!
    // TODO should be made thread safe (take a look at lock-free approaches)
    Handle<Entity> addEntity();

    std::size_t countLiveEntities() const;

private:
    Archetype & getArchetype(const TypeSet & aTypeSet);

    template <class T_component>
    Archetype & extendArchetype(const Archetype & aArchetype);

    template <class T_component>
    Archetype & restrictArchetype(const Archetype & aArchetype);

    template <class F_maker>
    Archetype & makeArchetypeIfAbsent(const TypeSet & aTargetTypeSet, F_maker aMakeCallback);

    EntityRecord & record(HandleKey aKey);

    void freeHandle(HandleKey aKey);

    template <class... VT_components>
    detail::QueryBackend<VT_components...> * getQueryBackend();

    // TODO Refactor with a fewer higher level classes, this is turning into a super class.
    HandleKey mNextHandle;
    std::map<HandleKey, EntityRecord> mHandleMap;
    std::deque<HandleKey> mFreedHandles;
    std::map<TypeSet, Archetype> mArchetypes;

    // TODO Ad 2022/07/08: Replace the TypeSequence by a TypeSet.
    // This will imply that all queries on the same set of components,
    // **independently of the order**, will share the same QueryBackend.
    // The main complication will be that the Query will need to statically sort the components
    // to instantiate their pointer to backend.
    std::map<TypeSequence, std::unique_ptr<detail::QueryBackendBase>> mQueryBackends;

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

    // The target archetype will grow by one: the size before insertion will be the inserted index.
    EntityIndex newIndex = targetArchetype.countEntities();
    initialRecord.mArchetype->move(initialRecord.mIndex, targetArchetype, mManager);

    // TODO ideally, we get rid of this test, so the implementations is as fast as possible
    // when the component is not present,
    // and it is suboptimal if it is already present.
    // The problem is with the push
    if (! initialRecord.mArchetype->has<T_component>()) [[likely]]
    {
        targetArchetype.push(std::move(aComponent));
    }
    else
    {
        // When the target archetype is the same as the source archetype
        // the target archetype will not grow in size, so decrement the new index.
        --newIndex;
        targetArchetype.get<T_component>(newIndex) = std::move(aComponent);
    }

    EntityRecord newRecord{
        .mArchetype = &targetArchetype,
        .mIndex = newIndex,
    };
    updateRecord(newRecord);
}


template <class T_component>
void Handle<Entity>::remove()
{
    EntityRecord initialRecord = record();

    Archetype & targetArchetype =
        mManager.restrictArchetype<T_component>(*initialRecord.mArchetype);

    // The target archetype will grow by one: the size before insertion will be the inserted index.
    EntityIndex newIndex = targetArchetype.countEntities();
    initialRecord.mArchetype->move(initialRecord.mIndex, targetArchetype, mManager);

    if (!initialRecord.mArchetype->has<T_component>()) [[unlikely]]
    {
        // When the target archetype is the same as the source archetype (i.e., the component was not present)
        // the target archetype will not grow in size, so decrement the new index.
        --newIndex;
    }

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
        archetype.countEntities(),
    };

    HandleKey key = mNextHandle++;
    // Has to be done after taking the entity count as index, for the new EntityRecord.
    archetype.pushKey(key);

    return Handle<Entity>{key, *this};
}


template <class T_component>
Archetype & EntityManager::extendArchetype(const Archetype & aArchetype)
{
    TypeSet targetTypeSet{aArchetype.getTypeSet()};
    targetTypeSet.insert(getId<T_component>());

    return makeArchetypeIfAbsent(targetTypeSet,
                                 std::bind(&Archetype::makeExtended<T_component>,
                                           std::cref(aArchetype)));
}


template <class T_component>
Archetype & EntityManager::restrictArchetype(const Archetype & aArchetype)
{
    TypeSet targetTypeSet{aArchetype.getTypeSet()};
    targetTypeSet.erase(getId<T_component>());

    return makeArchetypeIfAbsent(targetTypeSet,
                                 std::bind(&Archetype::makeRestricted<T_component>,
                                           std::cref(aArchetype)));
}


template <class F_maker>
Archetype & EntityManager::makeArchetypeIfAbsent(const TypeSet & aTargetTypeSet,
                                                 F_maker aMakeCallback)
{
    if (auto found = mArchetypes.find(aTargetTypeSet);
        found != mArchetypes.end())
    {
        return found->second;
    }
    else
    {
        Archetype & inserted = mArchetypes.emplace(aTargetTypeSet, aMakeCallback()).first->second;
        // Ask each QueryBackend if it is interested in the new archetype.
        for (auto & [_types, queryBackend] : mQueryBackends)
        {
            queryBackend->pushIfMatches(aTargetTypeSet, inserted);
        }
        return inserted;
    }
}


template <class... VT_components>
detail::QueryBackend<VT_components...> * EntityManager::getQueryBackend()
{
    TypeSequence backendKey = getTypeSequence<VT_components...>();
    detail::QueryBackendBase * backend;
    if (auto found = mQueryBackends.find(backendKey);
        found != mQueryBackends.end())
    {
        backend = found->second.get();
    }
    else
    {
        auto insertion = mQueryBackends.emplace(
            backendKey,
            std::make_unique<detail::QueryBackend<VT_components...>>(mArchetypes.begin(),
                                                                     mArchetypes.end()));
        backend = insertion.first->second.get();
    }

    // Assert that the underlying object is of the expected dynamic type.
    assert(dynamic_cast<detail::QueryBackend<VT_components...> *>(backend) != nullptr);

    return static_cast<detail::QueryBackend<VT_components...> *>(backend);
}


} // namespace ent
} // namespace ad

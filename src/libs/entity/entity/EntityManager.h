#pragma once

#include "Archetype.h"
#include "ArchetypeStore.h"
#include "Entity.h"
#include "entity/detail/QueryBackend.h"

#include <algorithm>
#include <deque>
#include <map>

#include <cstddef>


namespace ad {
namespace ent {


// TODO implement reuse of handle from the free list
// TODO implement handle free list hosted implicitly in a vector of handles
class EntityManager
{
    friend class Archetype;
    friend class Handle<Entity>;
    template <class...>
    friend class Query;
    friend class State;

    // To be implemented by the test application, allowing to access private parts.
    template <class>
    friend class Inspector;

    class InternalState
    {
        template <class>
        friend class Inspector;

    public:
        Handle<Entity> addEntity(EntityManager & aManager);

        std::size_t countLiveEntities() const;

        Handle<Archetype> getArchetypeHandle(const TypeSet & aTypeSet);

        template <class T_component>
        ArchetypeRecord extendArchetype(const Archetype & aArchetype);

        template <class T_component>
        ArchetypeRecord restrictArchetype(const Archetype & aArchetype);

        EntityRecord & record(HandleKey aKey);
        Archetype & archetype(Handle<Archetype> aHandle);

        void freeHandle(HandleKey aKey);

        template <class... VT_components>
        detail::QueryBackend<VT_components...> * getQueryBackend();

        // TODO This could be massively optimized by keeping a graph of transformations on the
        // archetypes, and storing the backend difference along the edges.
        // Basically, the edge would cache this information.
        /// \brief Return all QueryBackends that are present i aCompared, but not in aReference.
        std::vector<detail::QueryBackendBase *>
        getExtraQueryBackends(const Archetype & aCompared, const Archetype & aReference) const;

    private:
        template <class F_maker>
        ArchetypeRecord makeArchetypeIfAbsent(const TypeSet & aTargetTypeSet, F_maker aMakeCallback);

        // TODO Refactor with fewer higher level classes, this is turning into a super class.
        HandleKey mNextHandle;
        std::map<HandleKey, EntityRecord> mHandleMap;
        std::deque<HandleKey> mFreedHandles;

        ArchetypeStore mArchetypes;

        // TODO Ad 2022/07/08: Replace the TypeSequence by a TypeSet.
        // This will imply that all queries on the same set of components,
        // **independently of the order**, will share the same QueryBackend.
        // The main complication will be that the Query will need to statically sort the components
        // to instantiate their pointer to backend.
        std::map<TypeSequence, std::shared_ptr<detail::QueryBackendBase>> mQueryBackends;
    };

public:
    //EntityManager();

    /// \warning Thread unsafe!
    // TODO should be made thread safe (take a look at lock-free approaches)
    Handle<Entity> addEntity()
    { return mState.addEntity(*this); }

    std::size_t countLiveEntities() const
    { return mState.countLiveEntities(); }

    State saveState() const;
    void restoreState(State aState);

private:
    // Forward all externally used methods to state, so the state struct is hidden
    Handle<Archetype> getArchetypeHandle(const TypeSet & aTypeSet)
    { return mState.getArchetypeHandle(aTypeSet); }

    template <class T_component>
    ArchetypeRecord extendArchetype(const Archetype & aArchetype)
    { return mState.extendArchetype<T_component>(aArchetype); }

    template <class T_component>
    ArchetypeRecord restrictArchetype(const Archetype & aArchetype)
    { return mState.restrictArchetype<T_component>(aArchetype); }

    EntityRecord & record(HandleKey aKey)
    { return mState.record(aKey); }

    Archetype & archetype(Handle<Archetype> aHandle)
    { return mState.archetype(aHandle); }

    void freeHandle(HandleKey aKey)
    { return mState.freeHandle(aKey); }

    template <class... VT_components>
    detail::QueryBackend<VT_components...> * getQueryBackend()
    { return mState.getQueryBackend<VT_components...>(); }

    //// TODO This could be massively optimized by keeping a graph of transformations on the
    //// archetypes, and storing the backend difference along the edges.
    //// Basically, the edge would cache this information.
    ///// \brief Return all QueryBackends that are present i aCompared, but not in aReference.
    std::vector<detail::QueryBackendBase *>
    getExtraQueryBackends(const Archetype & aCompared, const Archetype & aReference) const
    { return mState.getExtraQueryBackends(aCompared, aReference); }

    InternalState mState;
};


class State
{
    friend class EntityManager;

public:
    explicit State(EntityManager::InternalState aState) :
        mState{std::move(aState)}
    {}

private:
    EntityManager::InternalState mState;
};


//
// Implementations
//
// NOTE: In this file because it needs EntityManager definition.
template <class T_component>
void Handle<Entity>::add(T_component aComponent)
{
    EntityRecord initialRecord = record();
    Handle<Archetype> initialArchetypeHandle = initialRecord.mArchetype;

    Handle<Archetype> targetArchetypeHandle =
        mManager.extendArchetype<T_component>(mManager.archetype(initialArchetypeHandle)).mHandle;
    Archetype & targetArchetype = mManager.archetype(targetArchetypeHandle);

    // The extend might have invalidate the archetype reference
    // So take it only now.
    Archetype & initialArchetype = mManager.archetype(initialArchetypeHandle);

    // The target archetype will grow by one: the size before insertion will be the inserted index.
    EntityIndex newIndex = targetArchetype.countEntities();
    initialArchetype.move(initialRecord.mIndex, targetArchetype, mManager);

    // TODO ideally, we get rid of this test, so the implementations is as fast as possible
    // when the component is not present,
    // and it is suboptimal if it is already present.
    // The problem is with the push
    if (! initialArchetype.has<T_component>()) [[likely]]
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
        .mArchetype = targetArchetypeHandle,
        .mIndex = newIndex,
    };
    updateRecord(newRecord);

    // Notify the query backends that match target archetype, but not source archetype,
    // that a new entity was added.
    auto addedBackends = mManager.getExtraQueryBackends(targetArchetype, initialArchetype);
    for (const auto & addedQuery : addedBackends)
    {
        addedQuery->signalEntityAdded(*this, newRecord);
    }
}


template <class T_component>
void Handle<Entity>::remove()
{
    EntityRecord initialRecord = record();
    Handle<Archetype> initialArchetypeHandle = initialRecord.mArchetype;

    Handle<Archetype> targetArchetypeHandle =
        mManager.restrictArchetype<T_component>(mManager.archetype(initialArchetypeHandle)).mHandle;
    Archetype & targetArchetype = mManager.archetype(targetArchetypeHandle);

    Archetype & initialArchetype = mManager.archetype(initialArchetypeHandle);
    // Notify the query backends that match source archetype, but not target archetype,
    // that the entity is being removed.
    auto removedBackends = mManager.getExtraQueryBackends(initialArchetype, targetArchetype);
    for (const auto & removedQuery : removedBackends)
    {
        removedQuery->signalEntityRemoved(*this, initialRecord);
    }

    // The target archetype will grow by one: the size before insertion will be the inserted index.
    EntityIndex newIndex = targetArchetype.countEntities();
    initialArchetype.move(initialRecord.mIndex, targetArchetype, mManager);

    if (!initialArchetype.has<T_component>()) [[unlikely]]
    {
        // When the target archetype is the same as the source archetype (i.e., the component was not present)
        // the target archetype will not grow in size, so decrement the new index.
        --newIndex;
    }

    EntityRecord newRecord{
        .mArchetype = targetArchetypeHandle,
        .mIndex = newIndex,
    };
    updateRecord(newRecord);
}


inline Handle<Entity> EntityManager::InternalState::addEntity(EntityManager & aManager)
{
    // We know the empty archetype is first in the vector
    std::pair<Archetype &, Handle<Archetype>> empty =  mArchetypes.getEmptyArchetype();

    mHandleMap.insert_or_assign(
        mNextHandle,
        EntityRecord{
            empty.second,
            empty.first.countEntities(),
        });

    HandleKey key = mNextHandle++;
    // Has to be done after taking the entity count as index, for the new EntityRecord.
    empty.first.pushKey(key);

    return Handle<Entity>{key, aManager};
}


template <class T_component>
ArchetypeRecord EntityManager::InternalState::extendArchetype(const Archetype & aArchetype)
{
    TypeSet targetTypeSet{aArchetype.getTypeSet()};
    targetTypeSet.insert(getId<T_component>());

    return makeArchetypeIfAbsent(targetTypeSet,
                                 std::bind(&Archetype::makeExtended<T_component>,
                                           std::cref(aArchetype)));
}


template <class T_component>
ArchetypeRecord EntityManager::InternalState::restrictArchetype(const Archetype & aArchetype)
{
    TypeSet targetTypeSet{aArchetype.getTypeSet()};
    targetTypeSet.erase(getId<T_component>());

    return makeArchetypeIfAbsent(targetTypeSet,
                                 std::bind(&Archetype::makeRestricted<T_component>,
                                           std::cref(aArchetype)));
}


// TODO move to the ArchetypeStore
template <class F_maker>
ArchetypeRecord EntityManager::InternalState::makeArchetypeIfAbsent(const TypeSet & aTargetTypeSet,
                                                                    F_maker aMakeCallback)
{
    auto & archetypes = mArchetypes.mArchetypes;
    auto & handleToArchetype = mArchetypes.mHandleToArchetype;

    if (auto found = archetypes.find(aTargetTypeSet);
        found != archetypes.end())
    {
        return found->second;
    }
    else
    {
        std::size_t insertedPosition = handleToArchetype.size();
        handleToArchetype.push_back(aMakeCallback());
        ArchetypeRecord inserted = archetypes.emplace(
            aTargetTypeSet,
            ArchetypeRecord{
                Handle<Archetype>{insertedPosition}
            })
            .first->second;
        // Ask each QueryBackend if it is interested in the new archetype.
        for (auto & [_types, queryBackend] : mQueryBackends)
        {
            queryBackend->pushIfMatches(aTargetTypeSet, inserted.mHandle, mArchetypes);
        }
        return inserted;
    }
}


template <class... VT_components>
detail::QueryBackend<VT_components...> * EntityManager::InternalState::getQueryBackend()
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
            std::make_unique<detail::QueryBackend<VT_components...>>(mArchetypes));
        backend = insertion.first->second.get();
    }

    // Assert that the underlying object is of the expected dynamic type.
    assert(dynamic_cast<detail::QueryBackend<VT_components...> *>(backend) != nullptr);

    return static_cast<detail::QueryBackend<VT_components...> *>(backend);
}


} // namespace ent
} // namespace ad

#pragma once

#include "Archetype.h"
#include "ArchetypeStore.h"
#include "Entity.h"
#include "QueryStore.h"

#include "detail/CloningPointer.h"
#include "detail/QueryBackend.h"

#include <algorithm>
#include <deque>
#include <map>

#include <cstddef>


namespace ad {
namespace ent {

class State;
// TODO implement reuse of handle from the free list
// TODO implement handle free list hosted implicitly in a vector of handles
class EntityManager
{
    friend class Archetype;
    friend class Handle<Archetype>;
    friend class Handle<Entity>;
    template <class...> friend class Query;
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

        Handle<Archetype> getArchetypeHandle(const TypeSet & aTypeSet, EntityManager & aManager);

        template <class T_component>
        HandleKey<Archetype> extendArchetype(const Archetype & aArchetype);

        template <class T_component>
        HandleKey<Archetype> restrictArchetype(const Archetype & aArchetype);

        EntityRecord & record(HandleKey<Entity> aKey);
        Archetype & archetype(HandleKey<Archetype> aHandle);

        void freeHandle(HandleKey<Entity> aKey);

        template <class... VT_components>
        detail::QueryBackend<VT_components...> * getQueryBackend();

        template <class... VT_components>
        detail::QueryBackend<VT_components...> * queryBackend(TypeSequence aQueryType);

        // TODO This could be massively optimized by keeping a graph of transformations on the
        // archetypes, and storing the backend difference along the edges.
        // Basically, the edge would cache this information.
        /// \brief Return all QueryBackends that are present i aCompared, but not in aReference.
        std::vector<detail::QueryBackendBase *>
        getExtraQueryBackends(const Archetype & aCompared, const Archetype & aReference) const;

    private:
        template <class F_maker>
        HandleKey<Archetype> makeArchetypeIfAbsent(const TypeSet & aTargetTypeSet,
                                           F_maker && aMakeCallback);

        // TODO Refactor the Handle<Entity> related members into a coherent separate class.
        HandleKey<Entity> mNextHandle;
        std::map<HandleKey<Entity>, EntityRecord> mHandleMap;
        std::deque<HandleKey<Entity>> mFreedHandles;

        // This must appear BEFORE the archetypes, so QueryBackends are destructed AFTER Archetypes:
        // Archetypes might store Queries, whose dtor will stop listening to events by calling
        // QueryBackend methods.
        QueryStore mQueryBackends;

        ArchetypeStore mArchetypes;
    };

public:
    /// \warning Thread unsafe!
    // TODO #thread P1: should be made thread safe (take a look at lock-free approaches)
    // because this operation is not deferred (so parallel jobs could be doing it concurrently)
    // An idea to evaluate: could it be lock-free via read-modify-write?
    // see: https://preshing.com/20120612/an-introduction-to-lock-free-programming/
    Handle<Entity> addEntity()
    { return mState->addEntity(*this); }

    std::size_t countLiveEntities() const
    { return mState->countLiveEntities(); }

    /// \note: Not const, since it actually re-allocate the internal state
    State saveState();
    void restoreState(const State & aState);

private:
    // Forward all externally used methods to state, so the state struct is hidden
    Handle<Archetype> getArchetypeHandle(const TypeSet & aTypeSet)
    { return mState->getArchetypeHandle(aTypeSet, *this); }

    template <class T_component>
    HandleKey<Archetype> extendArchetype(const Archetype & aArchetype)
    { return mState->extendArchetype<T_component>(aArchetype); }

    template <class T_component>
    HandleKey<Archetype> restrictArchetype(const Archetype & aArchetype)
    { return mState->restrictArchetype<T_component>(aArchetype); }

    EntityRecord & record(HandleKey<Entity> aKey)
    { return mState->record(aKey); }

    Archetype & archetype(HandleKey<Archetype> aHandle)
    { return mState->archetype(aHandle); }

    void freeHandle(HandleKey<Entity> aKey)
    { return mState->freeHandle(aKey); }

    template <class... VT_components>
    detail::QueryBackend<VT_components...> * getQueryBackend()
    { return mState->getQueryBackend<VT_components...>(); }

    template <class... VT_components>
    detail::QueryBackend<VT_components...> * queryBackend(TypeSequence aQueryType)
    { return mState->queryBackend<VT_components...>(aQueryType); }

    //// TODO This could be massively optimized by keeping a graph of transformations on the
    //// archetypes, and storing the backend difference along the edges.
    //// Basically, the edge would cache this information.
    ///// \brief Return all QueryBackends that are present i aCompared, but not in aReference.
    std::vector<detail::QueryBackendBase *>
    getExtraQueryBackends(const Archetype & aCompared, const Archetype & aReference) const
    { return mState->getExtraQueryBackends(aCompared, aReference); }

    std::unique_ptr<InternalState> mState = std::make_unique<InternalState>();
};


class State
{
    friend class EntityManager;

public:
    // Usefull to implement storage in preallocated std::array
    State() = default;

    explicit State(std::unique_ptr<EntityManager::InternalState> aState) :
        mState{std::move(aState)}
    {}

private:
    std::unique_ptr<EntityManager::InternalState> mState;
};


//
// Implementations
//
// NOTE: In this file because it needs EntityManager definition.
template <class T_component>
void Handle<Entity>::add(T_component aComponent)
{
    EntityRecord initialRecord = record();
    HandleKey<Archetype> initialArchetypeKey = initialRecord.mArchetype;

    HandleKey<Archetype> targetArchetypeKey =
        mManager->extendArchetype<T_component>(mManager->archetype(initialArchetypeKey));
    Archetype & targetArchetype = mManager->archetype(targetArchetypeKey);

    // The extend might have invalidate the archetype reference
    // So take it only now.
    Archetype & initialArchetype = mManager->archetype(initialArchetypeKey);

    // The target archetype will grow by one: the size before insertion will be the inserted index.
    EntityIndex newIndex = targetArchetype.countEntities();
    initialArchetype.move(initialRecord.mIndex, targetArchetype, *mManager);

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
        .mArchetype = targetArchetypeKey,
        .mIndex = newIndex,
    };
    updateRecord(newRecord);

    // Notify the query backends that match target archetype, but not source archetype,
    // that a new entity was added.
    auto addedBackends = mManager->getExtraQueryBackends(targetArchetype, initialArchetype);
    for (const auto & addedQuery : addedBackends)
    {
        addedQuery->signalEntityAdded(*this, newRecord);
    }
}


template <class T_component>
void Handle<Entity>::remove()
{
    EntityRecord initialRecord = record();
    HandleKey<Archetype> initialArchetypeKey = initialRecord.mArchetype;

    HandleKey<Archetype> targetArchetypeKey =
        mManager->restrictArchetype<T_component>(mManager->archetype(initialArchetypeKey));
    Archetype & targetArchetype = mManager->archetype(targetArchetypeKey);

    Archetype & initialArchetype = mManager->archetype(initialArchetypeKey);
    // Notify the query backends that match source archetype, but not target archetype,
    // that the entity is being removed.
    auto removedBackends = mManager->getExtraQueryBackends(initialArchetype, targetArchetype);
    for (const auto & removedQuery : removedBackends)
    {
        removedQuery->signalEntityRemoved(*this, initialRecord);
    }

    // The target archetype will grow by one: the size before insertion will be the inserted index.
    EntityIndex newIndex = targetArchetype.countEntities();
    initialArchetype.move(initialRecord.mIndex, targetArchetype, *mManager);

    if (!initialArchetype.has<T_component>()) [[unlikely]]
    {
        // When the target archetype is the same as the source archetype (i.e., the component was not present)
        // the target archetype will not grow in size, so decrement the new index.
        --newIndex;
    }

    EntityRecord newRecord{
        .mArchetype = targetArchetypeKey,
        .mIndex = newIndex,
    };
    updateRecord(newRecord);
}


inline Handle<Entity> EntityManager::InternalState::addEntity(EntityManager & aManager)
{
    // We know the empty archetype is first in the vector
    std::pair<Archetype &, HandleKey<Archetype>> empty =  mArchetypes.getEmptyArchetype();

    mHandleMap.insert_or_assign(
        mNextHandle,
        EntityRecord{
            empty.second,
            empty.first.countEntities(),
        });

    HandleKey<Entity> key = mNextHandle++;
    // Has to be done after taking the entity count as index, for the new EntityRecord.
    empty.first.pushKey(key);

    return Handle<Entity>{key, aManager};
}


template <class T_component>
HandleKey<Archetype> EntityManager::InternalState::extendArchetype(const Archetype & aArchetype)
{
    TypeSet targetTypeSet{aArchetype.getTypeSet()};
    targetTypeSet.insert(getId<T_component>());

    return makeArchetypeIfAbsent(targetTypeSet,
                                 std::bind(&Archetype::makeExtended<T_component>,
                                           std::cref(aArchetype)));
}


template <class T_component>
HandleKey<Archetype> EntityManager::InternalState::restrictArchetype(const Archetype & aArchetype)
{
    TypeSet targetTypeSet{aArchetype.getTypeSet()};
    targetTypeSet.erase(getId<T_component>());

    return makeArchetypeIfAbsent(targetTypeSet,
                                 std::bind(&Archetype::makeRestricted<T_component>,
                                           std::cref(aArchetype)));
}


template <class F_maker>
HandleKey<Archetype> EntityManager::InternalState::makeArchetypeIfAbsent(const TypeSet & aTargetTypeSet,
                                                                      F_maker && aMakeCallback)
{
    auto [handle, didInsert] =
            mArchetypes.makeIfAbsent(aTargetTypeSet, std::forward<F_maker>(aMakeCallback));
    if(didInsert)
    {
        for (auto & [_types, queryBackend] : mQueryBackends)
        {
            queryBackend->pushIfMatches(aTargetTypeSet, handle, mArchetypes);
        }
    }
    return handle;
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


template <class... VT_components>
detail::QueryBackend<VT_components...> *
EntityManager::InternalState::queryBackend(TypeSequence aQueryType)
{
    return static_cast<detail::QueryBackend<VT_components...> *>(mQueryBackends.at(aQueryType).get());
}


} // namespace ent
} // namespace ad

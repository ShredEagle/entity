#pragma once

#include "Archetype.h"
#include "ArchetypeStore.h"
#include "detail/CloningPointer.h"
#include "detail/QueryBackend.h"
#include "Entity.h"
#include "handy/StringId.h"
#include "QueryStore.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <iostream>
#include <map>
#include <sstream>

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
    template <class...>
    friend class Query;
    friend class State;

    // To be implemented by the test application, allowing to access private
    // parts.
    template <class>
    friend class Inspector;

    class InternalState
    {
        template <class>
        friend class Inspector;

    public:
        Handle<Entity> addEntity(EntityManager & aManager, const char * aName);

        std::size_t countLiveEntities() const;

        Handle<Archetype> getArchetypeHandle(const TypeSet & aTypeSet,
                                             EntityManager & aManager);

        template <class T_component>
        HandleKey<Archetype> extendArchetype(const Archetype & aArchetype);

        template <class T_component>
        HandleKey<Archetype> restrictArchetype(const Archetype & aArchetype);

        EntityRecord & record(HandleKey<Entity> aKey);

        /// \brief Return the complete HandleKey currently associated with the
        /// index subpart of `aKey`. \details mHandleMap is only comparing the
        /// index part of the HandleKey, which means this function will return
        /// an HandleKey with the same index, but not necessarily the same
        /// generation. This is usefull to test an Handle validity.
        const HandleKey<Entity> & keyForIndex(HandleKey<Entity> aKey);

        Archetype & archetype(HandleKey<Archetype> aHandle);

        const char * name(HandleKey<Entity> aHandle) const;

        Handle<Entity> handleFromName(const handy::StringId & aNameId,
                                      EntityManager & aManager) const;

        void freeHandle(HandleKey<Entity> aKey);

        template <class... VT_components>
        detail::QueryBackend<VT_components...> * getQueryBackend();

        template <class... VT_components>
        detail::QueryBackend<VT_components...> *
        queryBackend(TypeSequence aQueryType);

        std::set<detail::QueryBackendBase *>
        getQueryBackendSet(const Archetype & aArchetype) const;

        void forEachHandle(
            std::function<void(Handle<Entity>, const char *)> aCallback,
            EntityManager & aManager);

        // TODO This could be massively optimized by keeping a graph of
        // transformations on the archetypes, and storing the backend difference
        // along the edges. Basically, the edge would cache this information.
        /// \brief Return all QueryBackends that are present in aCompared, but
        /// not in aReference.
        std::vector<detail::QueryBackendBase *>
        getExtraQueryBackends(const Archetype & aCompared,
                              const Archetype & aReference) const;

        void insertInvalidHandleKey();

    private:
        template <class F_maker>
        HandleKey<Archetype>
        makeArchetypeIfAbsent(const TypeSet & aTargetTypeSet,
                              F_maker && aMakeCallback);

        /// \brief Get an available handle. Prefers returning from the free list
        /// if not empty.
        HandleKey<Entity> getAvailableHandle();

        // TODO Refactor the Handle<Entity> related members into a coherent
        // separate class.
        HandleKey<Entity> mNextHandle{
            HandleKey<Entity>::MakeFirst()}; // Initially, the first handle is
                                             // the next handle.

        // Note: Uses a custom comparison, only testing the index part of the
        // HandleKey (not the generation). This is to be consistent with the
        // fact that different generations of the same index should be placed at
        // the same key position from the map perspective.
        std::map<HandleKey<Entity>, EntityRecord, HandleKey<Entity>::LessIndex>
            mHandleMap;
        std::map<handy::StringId, HandleKey<Entity>> mHandleByNameMap;
        std::deque<HandleKey<Entity>> mFreedHandles;

        // This must appear BEFORE the archetypes, so QueryBackends are
        // destructed AFTER Archetypes: Archetypes might store Queries, whose
        // dtor will stop listening to events by calling QueryBackend methods.
        QueryStore mQueryBackends;

        ArchetypeStore mArchetypes;
    };

public:
    /// \warning Thread unsafe!
    // TODO #thread P1: should be made thread safe (take a look at lock-free
    // approaches) because this operation is not deferred (so parallel jobs
    // could be doing it concurrently) An idea to evaluate: could it be
    // lock-free via read-modify-write? see:
    // https://preshing.com/20120612/an-introduction-to-lock-free-programming/
    Handle<Entity> addEntity(const char * aName = nullptr)
    {
        return mState->addEntity(*this, aName);
    }

    Handle<Entity> addBlueprint(const char * aName = nullptr)
    {
        auto handle = mState->addEntity(*this, aName);
        {
            Phase phase;
            handle.get(phase)->add(Blueprint{});
        }
        return handle;
    }

    Handle<Entity> createFromBlueprint(Handle<Entity> aBlueprint,
                                       const char * aName);

    std::size_t countLiveEntities() const
    {
        return mState->countLiveEntities();
    }

    /// \note: Not const, since it actually re-allocate the internal state
    State saveState();
    void restoreState(const State & aState);

    // \note: For debug purpose we need a way to access all currently valid
    // handles
    void
    forEachHandle(std::function<void(Handle<Entity>, const char *)> aCallback);

    Handle<Entity> handleFromName(const std::string & aName)
    {
        return mState->handleFromName(handy::StringId(aName), *this);
    }

private:
    // Forward all externally used methods to state, so the state struct is
    // hidden
    Handle<Archetype> getArchetypeHandle(const TypeSet & aTypeSet)
    {
        return mState->getArchetypeHandle(aTypeSet, *this);
    }

    template <class T_component>
    HandleKey<Archetype> extendArchetype(const Archetype & aArchetype)
    {
        return mState->extendArchetype<T_component>(aArchetype);
    }

    template <class T_component>
    HandleKey<Archetype> restrictArchetype(const Archetype & aArchetype)
    {
        return mState->restrictArchetype<T_component>(aArchetype);
    }

    EntityRecord & record(HandleKey<Entity> aKey)
    {
        return mState->record(aKey);
    }

    const HandleKey<Entity> & keyForIndex(HandleKey<Entity> aKey)
    {
        return mState->keyForIndex(aKey);
    }

    Archetype & archetype(HandleKey<Archetype> aHandle)
    {
        return mState->archetype(aHandle);
    }

    const char * name(HandleKey<Entity> aHandle) const
    {
        return mState->name(aHandle);
    }

    void freeHandle(HandleKey<Entity> aKey) { return mState->freeHandle(aKey); }

    template <class... VT_components>
    detail::QueryBackend<VT_components...> * getQueryBackend()
    {
        return mState->getQueryBackend<VT_components...>();
    }

    template <class... VT_components>
    detail::QueryBackend<VT_components...> *
    queryBackend(TypeSequence aQueryType)
    {
        return mState->queryBackend<VT_components...>(aQueryType);
    }

    std::set<detail::QueryBackendBase *>
    getQueryBackendSet(const Archetype & aArchetype) const
    {
        return mState->getQueryBackendSet(aArchetype);
    }

    std::vector<detail::QueryBackendBase *>
    getExtraQueryBackends(const Archetype & aCompared,
                          const Archetype & aReference) const
    {
        return mState->getExtraQueryBackends(aCompared, aReference);
    }

    // Only used by the default constructor of Handle<Entity>.
    // See Handle<Entity>::Handle() for context.
    static EntityManager & getEmptyHandleEntityManager();

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
// NOTE: Implemented this file because it needs EntityManager definition.
template <class T_component>
void Handle<Entity>::add(T_component aComponent)
{
    EntityRecord initialRecord = record();
    HandleKey<Archetype> initialArchetypeKey = initialRecord.mArchetype;

    HandleKey<Archetype> targetArchetypeKey =
        mManager->extendArchetype<T_component>(
            mManager->archetype(initialArchetypeKey));
    Archetype & targetArchetype = mManager->archetype(targetArchetypeKey);

    // The extend might have invalidate the archetype reference
    // So take it only now.
    Archetype & initialArchetype = mManager->archetype(initialArchetypeKey);

    // The target archetype will grow by one: the size before insertion will be
    // the inserted index.
    EntityIndex newIndex = targetArchetype.countEntities();
    initialArchetype.move(initialRecord.mIndex, targetArchetype, *mManager);

    EntityRecord newRecord{
        .mArchetype = targetArchetypeKey,
        .mIndex = newIndex,
        .mNamePtr = initialRecord.mNamePtr,
    };

    // TODO ideally, we get rid of this test, so the implementations is as fast
    // as possible when the component is not present, and it is suboptimal if it
    // is already present. The problem is with the push vs assign, and the
    // difference in record index.
    if (!initialArchetype.has<T_component>()) [[likely]]
    {
        targetArchetype.push(std::move(aComponent));
        updateRecord(newRecord);
    }
    else
    {
        // If the component was already present, move() left the entity at the
        // same index, in the same archetype. We need to replace the value, but
        // there is no need to update the EntityRecord.
        targetArchetype.get<T_component>(initialRecord.mIndex) =
            std::move(aComponent);
        newRecord.mIndex =
            initialRecord
                .mIndex; // Just for consistency, this is not used later.

        // Discussion from 2023/05/27: even there might be legitimate cases
        // to get in this scenario, disallow it for the moment.
        // If it shows up, we can allow it via a parameter / distinct member
        // function.
        // --
        // But it would break tests.
        // assert(false);
    }

    // Notify the query backends that match target archetype, but not source
    // archetype, that a new entity was added.
    auto addedBackends =
        mManager->getExtraQueryBackends(targetArchetype, initialArchetype);
    for (const auto & addedQuery : addedBackends)
    {
        // We should not pass there if this is a redundant add().
        assert(!initialArchetype.has<T_component>());
        addedQuery->signalEntityAdded(*this, newRecord);
    }

#if defined(ENTITY_SANITIZE)
    assert(initialArchetype.verifyHandlesConsistency(*mManager));
    assert(targetArchetype.verifyHandlesConsistency(*mManager));
#endif
}

template <class T_component>
void Handle<Entity>::remove()
{
    EntityRecord initialRecord = record();
    HandleKey<Archetype> initialArchetypeKey = initialRecord.mArchetype;

    HandleKey<Archetype> targetArchetypeKey =
        mManager->restrictArchetype<T_component>(
            mManager->archetype(initialArchetypeKey));
    Archetype & targetArchetype = mManager->archetype(targetArchetypeKey);

    Archetype & initialArchetype = mManager->archetype(initialArchetypeKey);

    // Notify the query backends that match source archetype, but not target
    // archetype, that the entity is being removed.
    auto removedBackends =
        mManager->getExtraQueryBackends(initialArchetype, targetArchetype);
    for (const auto & removedQuery : removedBackends)
    {
        removedQuery->signalEntityRemoved(*this, initialRecord);
    }

    // The target archetype will grow by one: the size before insertion will be
    // the inserted index.
    EntityIndex newIndex = targetArchetype.countEntities();
    initialArchetype.move(initialRecord.mIndex, targetArchetype, *mManager);

    // If the component was not present, move() left the entity at the same
    // index, nothing to be done.
    if (initialArchetype.has<T_component>()) [[likely]]
    {
        EntityRecord newRecord{
            .mArchetype = targetArchetypeKey,
            .mIndex = newIndex,
            .mNamePtr = initialRecord.mNamePtr,
        };
        updateRecord(newRecord);
    }
    else
    {
        // If the component was not present, move() left the entity at the same
        // index, in the same archetype. There is nothing to do in this
        // situation.

        // Discussion from 2023/05/27: even there might be legitimate cases
        // to get in this scenario, disallow it for the moment.
        // If it shows up, we can allow it via a parameter / distinct member
        // function.
        // --
        // But it would break tests.
        // assert(false);
    }

#if defined(ENTITY_SANITIZE)
    assert(initialArchetype.verifyHandlesConsistency(*mManager));
    assert(targetArchetype.verifyHandlesConsistency(*mManager));
#endif
}

inline Handle<Entity>
EntityManager::InternalState::addEntity(EntityManager & aManager,
                                        const char * aName)
{
    // We know the empty archetype is first in the vector
    std::pair<Archetype &, HandleKey<Archetype>> emptyArchetype =
        mArchetypes.getEmptyArchetype();

    HandleKey<Entity> key = getAvailableHandle();

    if (aName == nullptr)
    {
        std::stringstream newName;
        newName << "Entity " << key;
        aName = newName.str().c_str();
    }

    mHandleMap.insert_or_assign(key, EntityRecord{
                                         emptyArchetype.second,
                                         emptyArchetype.first.countEntities(),
                                         std::make_shared<std::string>(aName),
                                     });

    handy::StringId nameId{aName};
#ifndef NDEBUG
    if (mHandleByNameMap.contains(nameId)
        && mHandleByNameMap.at(nameId).isLastGeneration())
    {
        std::cerr << "There is already a entity with name: " << aName
                  << std::endl;
        assert(false);
    }
#endif

    mHandleByNameMap.insert_or_assign(nameId, key);
    // Has to be done after taking the entity count as index, for the new
    // EntityRecord.
    emptyArchetype.first.pushKey(key);

    return Handle<Entity>{key, aManager};
}

template <class T_component>
HandleKey<Archetype>
EntityManager::InternalState::extendArchetype(const Archetype & aArchetype)
{
    TypeSet targetTypeSet{aArchetype.getTypeSet()};
    targetTypeSet.insert(getId<T_component>());

    return makeArchetypeIfAbsent(
        targetTypeSet, std::bind(&Archetype::makeExtended<T_component>,
                                 std::cref(aArchetype)));
}

template <class T_component>
HandleKey<Archetype>
EntityManager::InternalState::restrictArchetype(const Archetype & aArchetype)
{
    TypeSet targetTypeSet{aArchetype.getTypeSet()};
    targetTypeSet.erase(getId<T_component>());

    return makeArchetypeIfAbsent(
        targetTypeSet, std::bind(&Archetype::makeRestricted<T_component>,
                                 std::cref(aArchetype)));
}

template <class F_maker>
HandleKey<Archetype> EntityManager::InternalState::makeArchetypeIfAbsent(
    const TypeSet & aTargetTypeSet, F_maker && aMakeCallback)
{
    auto [handle, didInsert] = mArchetypes.makeIfAbsent(
        aTargetTypeSet, std::forward<F_maker>(aMakeCallback));
    if (didInsert)
    {
        for (auto & [_types, queryBackend] : mQueryBackends)
        {
            queryBackend->pushIfMatches(aTargetTypeSet, handle, mArchetypes);
        }
    }
    return handle;
}

template <class... VT_components>
detail::QueryBackend<VT_components...> *
EntityManager::InternalState::getQueryBackend()
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
            std::make_unique<detail::QueryBackend<VT_components...>>(
                mArchetypes));
        backend = insertion.first->second.get();
    }

    // Assert that the underlying object is of the expected dynamic type.
    assert(dynamic_cast<detail::QueryBackend<VT_components...> *>(backend)
           != nullptr);

    return static_cast<detail::QueryBackend<VT_components...> *>(backend);
}

template <class... VT_components>
detail::QueryBackend<VT_components...> *
EntityManager::InternalState::queryBackend(TypeSequence aQueryType)
{
    return static_cast<detail::QueryBackend<VT_components...> *>(
        mQueryBackends.at(aQueryType).get());
}

} // namespace ent
} // namespace ad

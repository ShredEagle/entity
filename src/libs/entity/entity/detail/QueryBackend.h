#pragma once


#include "HandledStore.h"

#include <entity/Archetype.h>
#include <entity/ArchetypeStore.h>
#include <entity/Entity.h>

#include <algorithm>
#include <concepts>
#include <list>


namespace ad {
namespace ent {
namespace detail {


class QueryBackendBase
{
public:
    virtual ~QueryBackendBase() = default;

    virtual std::unique_ptr<QueryBackendBase> clone() const = 0;

    virtual void pushIfMatches(const TypeSet & aCandidateTypeSet,
                               HandleKey<Archetype> aCandidate,
                               const ArchetypeStore & aStore) = 0;
    virtual void signalEntityAdded(Handle<Entity> aEntity, const EntityRecord & aRecord) = 0;
    virtual void signalEntityRemoved(Handle<Entity> aEntity, const EntityRecord & aRecord) = 0;
};


// TODO Ad 2022/07/13 #state p1: FRAMESAVE Ensure the Listening dtor of a discarded frame does not
// stop the listening in active frames! (i.e. the opposite of the usual problem, we have to make
// sure the effects are not accross frames). Yet the copy in a new frame should be able to delete
// in the new frame...
// TODO Ad 2022/07/13: Replace with Handy guard, e.g. using Listening = Guard;
class [[nodiscard]] Listening
{
    using Callback_t = std::function<void(QueryBackendBase &)>;

public:
    template <class F_guard>
    requires std::invocable<F_guard, QueryBackendBase &>
    explicit Listening(QueryBackendBase * aBackend, F_guard && aGuard) :
        mBackend{aBackend},
        mGuard{std::make_unique<Callback_t>(std::forward<F_guard>(aGuard))}
    {}

    Listening(const Listening & aRhs, QueryBackendBase * aBackend) :
        mGuard{std::make_unique<Callback_t>(*aRhs.mGuard)},
        mBackend{aBackend}
    {
        // If both Listening remove from the same backend, there is a logic error.
        assert(mBackend != aRhs.mBackend);
    }

    Listening(Listening &&) = default;
    Listening & operator=(Listening &&) = default;

    ~Listening()
    {
        if (mGuard)
        {
            std::invoke(*mGuard, *mBackend);
        }
    }

    void redirect( QueryBackendBase * aBackend)
    {
        mBackend = aBackend;
    }

private:
    std::unique_ptr<Callback_t> mGuard;
    QueryBackendBase * mBackend;
};


// Implementation note: instances of this templates are hosted by the EntityManager,
// which maintains then in a map.
// There should be only one instance (for a given VT_components...) per EntityManager.
// This allows to cache queries (all identicaly queries share the single backend instance),
// and the EntityManager is responsible for keeping the backends up to date.
template <class... VT_components>
class QueryBackend : public QueryBackendBase
{
    // This class is an implementation detail, client should not see it.
public:
    struct MatchedArchetype
    {
        MatchedArchetype(HandleKey<Archetype> aArchetype, const ArchetypeStore & aStore);

        HandleKey<Archetype> mArchetype;
        // IMPORTANT: Cannot cache the pointer to components' storage
        // because the storage is currently a vector (i.e. prone to relocation)
        // This would also complicate the frame-state implementation.
        //std::tuple<VT_components *...> mComponents;
        std::tuple<StorageIndex<VT_components>...> mComponentIndices;
    };

    using AddedEntityCallback = std::function<void(VT_components &...)>;
    using RemovedEntityCallback = std::function<void(VT_components &...)>;

    QueryBackend(const ArchetypeStore & aArchetypes);

    std::unique_ptr<QueryBackendBase> clone() const final;

    template <class F_function>
    Listening listenEntityAdded(F_function && aCallback);

    template <class F_function>
    Listening listenEntityRemoved(F_function && aCallback);

    void pushIfMatches(const TypeSet & aCandidateTypeSet,
                       HandleKey<Archetype> aCandidate,
                       const ArchetypeStore & aStore) final;

    void signalEntityAdded(Handle<Entity> aEntity, const EntityRecord & aRecord) final;

    void signalEntityRemoved(Handle<Entity> aEntity, const EntityRecord & aRecord) final;

    template <class T_range>
    void signal_impl(Handle<Entity> aEntity,
                     const EntityRecord & aRecord,
                     const T_range & aListeners) const;

    static const TypeSet & GetTypeSet();

    std::vector<MatchedArchetype> mMatchingArchetypes;
    // A list, because we can delete in the middle, and it should not invalidate other iterators.
    HandledStore<AddedEntityCallback> mAddListeners;
    HandledStore<RemovedEntityCallback> mRemoveListeners;
};


/// \brief Invoke a callback on a matched archetype, for a given entity in the archetype.
template <class... VT_components, class F_callback>
void invoke(F_callback aCallback,
            std::tuple<Storage<VT_components> & ...> aStorages,
            EntityIndex aIndexInArchetype)
{
    aCallback(std::get<Storage<VT_components> &>(aStorages).mArray[aIndexInArchetype]...);
}


template <class... VT_components, class F_callback>
void invokePair(F_callback aCallback,
                std::tuple<Storage<VT_components> & ...> aStoragesA,
                EntityIndex aIndexInArchetypeA,
                std::tuple<Storage<VT_components> & ...> aStoragesB,
                EntityIndex aIndexInArchetypeB)
{
    aCallback(std::get<Storage<VT_components> &>(aStoragesA).mArray[aIndexInArchetypeA]...,
              std::get<Storage<VT_components> &>(aStoragesB).mArray[aIndexInArchetypeB]...);
}


//
// Implementations
//

template <class... VT_components>
QueryBackend<VT_components...>::MatchedArchetype::MatchedArchetype(
        HandleKey<Archetype> aArchetype,
        const ArchetypeStore & aStore) :
    mArchetype{aArchetype},
    mComponentIndices{aStore.get(mArchetype).template getStoreIndex<VT_components>()...}
{}


template <class... VT_components>
QueryBackend<VT_components...>::QueryBackend(const ArchetypeStore & aArchetypes)
{
    for(auto mapping = aArchetypes.beginMap(); mapping != aArchetypes.endMap(); ++mapping)
    {
        const auto & [typeSet, archetypeKey] = *mapping;
        pushIfMatches(typeSet, archetypeKey, aArchetypes);
    }
}


template <class... VT_components>
std::unique_ptr<QueryBackendBase> QueryBackend<VT_components...>::clone() const
{
    return std::make_unique<QueryBackend<VT_components...>>(*this);
}


template <class... VT_components>
template <class F_function>
Listening QueryBackend<VT_components...>::listenEntityAdded(F_function && aCallback)
{
    auto inserted = mAddListeners.emplace(std::forward<F_function>(aCallback));
    return Listening{
        this,
        [inserted](QueryBackendBase & aBackend)
        {
            static_cast<QueryBackend<VT_components...> &>(aBackend)
                .mAddListeners.erase(inserted);
        }};
}


template <class... VT_components>
template <class F_function>
Listening QueryBackend<VT_components...>::listenEntityRemoved(F_function && aCallback)
{
    auto inserted = mRemoveListeners.emplace(std::forward<F_function>(aCallback));
    return Listening{
        this,
        [inserted](QueryBackendBase & aBackend)
        {
            static_cast<QueryBackend<VT_components...> &>(aBackend)
                .mRemoveListeners.erase(inserted);
        }};
}


template <class... VT_components>
void QueryBackend<VT_components...>::pushIfMatches(const TypeSet & aCandidateTypeSet,
                                                   HandleKey<Archetype> aCandidate,
                                                   const ArchetypeStore & aStore)
{
    if(std::includes(aCandidateTypeSet.begin(), aCandidateTypeSet.end(),
                     GetTypeSet().begin(), GetTypeSet().end()))
    {
        mMatchingArchetypes.emplace_back(aCandidate, aStore);
    }
}


template <class... VT_components>
void QueryBackend<VT_components...>::signalEntityAdded(Handle<Entity> aEntity, const EntityRecord & aRecord)
{
    signal_impl(aEntity, aRecord, mAddListeners);
}


template <class... VT_components>
void QueryBackend<VT_components...>::signalEntityRemoved(Handle<Entity> aEntity, const EntityRecord & aRecord)
{
    signal_impl(aEntity, aRecord, mRemoveListeners);
}


template <class... VT_components>
template <class T_range>
void QueryBackend<VT_components...>::signal_impl(
        Handle<Entity> aEntity,
        const EntityRecord & aRecord,
        const T_range & aListeners) const
{
    auto found =
        std::find_if(mMatchingArchetypes.begin(),
                     mMatchingArchetypes.end(),
                     [&aRecord, &aEntity](const auto & aMatch) -> bool
                     {
                       return aMatch.mArchetype == aEntity.record().mArchetype;
                     });

    // TODO: Can be more efficient, we already have the EntityRecord.
    Archetype & archetype = aEntity.archetype();

    assert(found != mMatchingArchetypes.end());
    assert(aRecord.mIndex < found->mArchetype->countEntities());
    // TODO this if was introduced by the refactoring to allow vectorization
    // Can it be removed?
    if (!aListeners.empty())
    {
        // TODO factorize with the equivalent code in Query.h
        std::tuple<Storage<VT_components> & ...> storages =
            std::tie(
                archetype.getStorage(
                    std::get<StorageIndex<VT_components>>(found->mComponentIndices))...);
        for(auto & [_handle, callback] : aListeners)
        {
            invoke<VT_components...>(callback, storages, aRecord.mIndex);
        }
    }
}


template <class... VT_components>
const TypeSet & QueryBackend<VT_components...>::GetTypeSet()
{
    static TypeSet queryTypeSet = getTypeSet<VT_components...>();
    return queryTypeSet;
}


} // namespace detail
} // namespace ent
} // namespace ad

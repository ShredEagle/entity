#pragma once

#include "HandledStore.h"

#include <entity/Component.h>
#include <entity/HandleKey.h>
#include <entity/StorageIndex.h>
#include <entity/Handle.h>

#include <memory>
#include <functional>
#include <cassert>

namespace ad {
namespace ent {

class Archetype;
class ArchetypeStore;
struct EntityRecord;
class Entity;

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
//
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
        mGuard{std::make_unique<Callback_t>(std::forward<F_guard>(aGuard))},
        mBackend{aBackend}
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

/// \warning The order of MatchedArchetypes in the QueryBackend is implementation dependent:
/// When the QueryBackend is instantiated, it goes through the map of existing archetypes in the ArchetypeStore.
/// This map is sorted by TypeSet, whose order is implemenation dependent (it depends on type_id).
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

    using AddedEntityCallback = std::function<void(Handle<Entity>, VT_components &...)>;
    using RemovedEntityCallback = std::function<void(Handle<Entity>, VT_components &...)>;

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


}
}
}


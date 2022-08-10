#pragma once


#include "../Entity.h"
#include "../Archetype.h"

#include <algorithm>
#include <list>


namespace ad {
namespace ent {
namespace detail {


class QueryBackendBase
{
public:
    virtual ~QueryBackendBase() = default;

    virtual void pushIfMatches(const TypeSet & aCandidateTypeSet, Archetype & aCandidate) = 0;
    virtual void signalEntityAdded(Handle<Entity> aEntity, const EntityRecord & aRecord) = 0;
    virtual void signalEntityRemoved(Handle<Entity> aEntity, const EntityRecord & aRecord) = 0;
};


// Implementation note: instances of this templates are hosted by the EntityManager,
// which maintains then in amap.
// There should be only one instance (for a given VT_components...) per EntityManager.
// This allows to cache queries (all identicaly queries share the single backend instance),
// and the EntityManager is responsible for keeping the backends up to date.

// TODO Ad 2022/07/13: FRAMESAVE Ensure the Listening dtor of a discarded frame does not
// stop the listening in active frames! (i.e. the opposite of the usual problem, we have to make
// sure the effects are not accross frames). Yet the copy in a new frame should be able to delete
// in the new frame...
// TODO Ad 2022/07/13: Replace with Handy guard, e.g. using Listening = Guard;
class [[nodiscard]] Listening
{
public:
    template <class F_guard>
    explicit Listening(F_guard && aGuard) :
        mGuard{std::make_unique<std::function<void()>>(std::forward<F_guard>(aGuard))}
    {}

    Listening(const Listening &) = delete;
    Listening & operator=(const Listening &) = delete;

    Listening(Listening &&) = default;
    Listening & operator=(Listening &&) = default;

    ~Listening()
    {
        if(mGuard)
        {
            (*mGuard)();
        }
    }
private:
    std::unique_ptr<std::function<void()>> mGuard;
};


template <class... VT_components>
class QueryBackend : public QueryBackendBase
{
    // This class is an implementation detail, client should not see it.
public:
    struct MatchedArchetype
    {
        MatchedArchetype(Archetype * aArchetype);

        std::tuple<Storage<VT_components> & ...> getStorages() const;

        Archetype * mArchetype;
        // IMPORTANT: Cannot cache the pointer to components' storage
        // because the storage is currently a vector (i.e. prone to relocation)
        // This would also complicate the frame-state implementation.
        //std::tuple<VT_components *...> mComponents;
        std::tuple<StorageIndex<VT_components>...> mComponentIndices;
    };

    using AddedEntityCallback = std::function<void(VT_components &...)>;
    using RemovedEntityCallback = std::function<void(VT_components &...)>;

    template <class T_pairIterator>
    QueryBackend(T_pairIterator aFirst, T_pairIterator aLast);

    template <class F_function>
    Listening listenEntityAdded(F_function && aCallback);

    template <class F_function>
    Listening listenEntityRemoved(F_function && aCallback);

    void pushIfMatches(const TypeSet & aCandidateTypeSet, Archetype & aCandidate) final;

    void signalEntityAdded(Handle<Entity> aEntity, const EntityRecord & aRecord) final;

    void signalEntityRemoved(Handle<Entity> aEntity, const EntityRecord & aRecord) final;

    template <class T_range>
    void signal_impl(Handle<Entity> aEntity,
                     const EntityRecord & aRecord,
                     const T_range & aListeners) const;

    static const TypeSet & GetTypeSet();

    std::vector<MatchedArchetype> mMatchingArchetypes;
    // A list, because we can delete in the middle, and it should not invalidate other iterators.
    std::list<AddedEntityCallback> mAddListeners;
    std::list<RemovedEntityCallback> mRemoveListeners;
};


/// \brief Invoke a callback on a matched archetype, for a given entity in the archetype.
template <class... VT_components, class F_callback>
void invoke(F_callback aCallback,
            std::tuple<Storage<VT_components> & ...> aStorages,
            //const typename QueryBackend<VT_components...>::MatchedArchetype & aMatch,
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
QueryBackend<VT_components...>::MatchedArchetype::MatchedArchetype(Archetype * aArchetype) :
    mArchetype{aArchetype},
    mComponentIndices{mArchetype->getStoreIndex<VT_components>()...}
{}


template <class... VT_components>
std::tuple<Storage<VT_components> & ...> QueryBackend<VT_components...>::MatchedArchetype::getStorages() const
{
    return std::tie(mArchetype->getStorage(std::get<StorageIndex<VT_components>>(mComponentIndices))...);
}


template <class... VT_components>
template <class T_pairIterator>
QueryBackend<VT_components...>::QueryBackend(T_pairIterator aFirst, T_pairIterator aLast)
{
    for(/**/; aFirst != aLast; ++aFirst)
    {
        auto & [typeSet, archetype] = *aFirst;
        pushIfMatches(typeSet, archetype);
    }
}


template <class... VT_components>
template <class F_function>
Listening QueryBackend<VT_components...>::listenEntityAdded(F_function && aCallback)
{
    auto inserted = mAddListeners.emplace(mAddListeners.end(),
                                          std::forward<F_function>(aCallback));
    return Listening{[inserted, this]()
        {
            mAddListeners.erase(inserted);
        }};
}


template <class... VT_components>
template <class F_function>
Listening QueryBackend<VT_components...>::listenEntityRemoved(F_function && aCallback)
{
    auto inserted = mRemoveListeners.emplace(mRemoveListeners.end(),
                                             std::forward<F_function>(aCallback));
    return Listening{[inserted, this]()
        {
            mRemoveListeners.erase(inserted);
        }};
}


template <class... VT_components>
void QueryBackend<VT_components...>::pushIfMatches(const TypeSet & aCandidateTypeSet,
                                                   Archetype & aCandidate)
{
    if(std::includes(aCandidateTypeSet.begin(), aCandidateTypeSet.end(),
                     GetTypeSet().begin(), GetTypeSet().end()))
    {
        mMatchingArchetypes.push_back(&aCandidate);
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
                     [&aRecord](const auto & aMatch) -> bool
                     {
                       return aMatch.mArchetype == aRecord.mArchetype;
                     });

    assert(found != mMatchingArchetypes.end());
    assert(aRecord.mIndex < found->mArchetype->countEntities());
    if (!aListeners.empty())
    {
        std::tuple<Storage<VT_components> & ...> storages = found->getStorages();

        for(auto & callback : aListeners)
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

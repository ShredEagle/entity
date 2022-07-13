#pragma once


#include "../Entity.h"
#include "../Archetype.h"

#include <algorithm>


namespace ad {
namespace ent {
namespace detail {


class QueryBackendBase
{
public:
    virtual ~QueryBackendBase() = default;

    virtual void pushIfMatches(const TypeSet & aCandidateTypeSet, Archetype & aCandidate) = 0;
    virtual void signalEntityAdded(Handle<Entity> aEntity, const EntityRecord & aRecord) = 0;
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

        Archetype * mArchetype;
        // IMPORTANT: Cannot cache the pointer to components' storage
        // because the storage is currently a vector (i.e. prone to relocation)
        // This would also complicate the frame-state implementation.
        //std::tuple<VT_components *...> mComponents;
        std::tuple<StorageIndex<VT_components>...> mComponentIndices;
    };

    using AddedEntityCallback = std::function<void(VT_components &...)>;

    template <class T_pairIterator>
    QueryBackend(T_pairIterator aFirst, T_pairIterator aLast);

    template <class F_function>
    Listening listenEntityAdded(F_function && aCallback);

    void pushIfMatches(const TypeSet & aCandidateTypeSet, Archetype & aCandidate) final;

    void signalEntityAdded(Handle<Entity> aEntity, const EntityRecord & aRecord) final;

    static const TypeSet & GetTypeSet();

    std::vector<MatchedArchetype> mMatchingArchetypes;
    // A list, because we can delete in the middle, and it should not invalidate other iterators.
    std::list<AddedEntityCallback> mAddListeners;
};


/// \brief Invoke a callback on a matched archetype, for a given entity in the archetype.
template <class... VT_components, class F_callback>
void invoke(F_callback aCallback,
            const typename QueryBackend<VT_components...>::MatchedArchetype & aMatch,
            EntityIndex aIndexInArchetype)
{
    aCallback(aMatch.mArchetype->getStorage(
                std::get<StorageIndex<VT_components>>(aMatch.mComponentIndices))
                    .mArray[aIndexInArchetype]...);
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
    auto found =
        std::find_if(mMatchingArchetypes.begin(),
                     mMatchingArchetypes.end(),
                     [&aRecord](const auto & aMatch) -> bool
                     {
                       return aMatch.mArchetype == aRecord.mArchetype;
                     });
    assert(found != mMatchingArchetypes.end());
    assert(aRecord.mIndex < found->mArchetype->countEntities());

    for(auto & callback : mAddListeners)
    {
        invoke<VT_components...>(callback, *found, aRecord.mIndex);
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

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
template <class... VT_components>
class QueryBackend : public QueryBackendBase
{
    // This class is an implementation detail, client should not see it.
public:
    struct MatchedArchetype
    {
        MatchedArchetype(Archetype * aArchetype);

        Archetype * mArchetype;
        std::tuple<VT_components *...> mComponents;
    };

    using AddedEntityCallback = std::function<void(VT_components &...)>;

    template <class T_pairIterator>
    QueryBackend(T_pairIterator aFirst, T_pairIterator aLast);

    void pushIfMatches(const TypeSet & aCandidateTypeSet, Archetype & aCandidate) final;

    void signalEntityAdded(Handle<Entity> aEntity, const EntityRecord & aRecord) final;

    static const TypeSet & GetTypeSet();

    std::vector<MatchedArchetype> mMatchingArchetypes;
    std::vector<AddedEntityCallback> mAddListeners;
};


//
// Implementations
//
template <class... VT_components>
QueryBackend<VT_components...>::MatchedArchetype::MatchedArchetype(Archetype * aArchetype) :
    mArchetype{aArchetype},
    mComponents{mArchetype->begin<VT_components>()...}
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
        callback(std::get<VT_components *>(found->mComponents)[aRecord.mIndex]...);
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

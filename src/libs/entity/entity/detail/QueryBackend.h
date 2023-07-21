#pragma once

#include "QueryBackend-decl.h"

#include "HandledStore.h"
#include "Invoker.h"

#include <entity/Archetype.h>
#include <entity/ArchetypeStore.h>
#include <entity/Entity.h>

#include <algorithm>
#include <concepts>
#include <list>



namespace ad {
namespace ent {
namespace detail {


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
                     [&aEntity](const auto & aMatch) -> bool
                     {
                       return aMatch.mArchetype == aEntity.record().mArchetype;
                     });

    // TODO: Can be more efficient, we already have the EntityRecord.
    Archetype & archetype = aEntity.archetype();

    assert(found != mMatchingArchetypes.end());
    assert(aRecord.mIndex < archetype.countEntities());
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
            // It is currently hardcoded that the signals' callback are taking all components, in order.
            // (franz): Added Handle 
            Invoker<std::tuple<Handle<Entity>, VT_components...>>::template invoke<VT_components...>(
                    callback,
                    aEntity, storages, aRecord.mIndex);
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

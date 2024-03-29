#pragma once

#include "Component.h"
#include "EntityManager.h"

#include "detail/Invoker.h"
#include "detail/QueryBackend.h"

#include <handy/FunctionTraits.h>
#if defined(ENTITY_SANITIZE)
#include <handy/Guard.h>
#endif

#include <numeric>


namespace ad {
namespace ent {

// Implementation note: this class shares the QueryBackend with all its instantiation
// (with matching components's types) for the same EntityManager.
/// \brief This is the class used by client code to query entities with corresponding components.
template <class... VT_components>
class Query
{
    template <class>
    friend class Storage;

public:
    /// \brief Instantiate the query for the provided EntityManager.
    ///
    /// Caching of queries will be handled automatically.
    Query(EntityManager & aManager);

    // TODO Should be private (and only allowed to be called by Storage<Query> cloning function
    // Yet the actual copy ctor is called deep within the stl, something we do not want to befriend.
    Query(const Query & aRhs);
    Query & operator=(const Query & aRhs);

    Query(Query && aRhs) = default;
    Query & operator=(Query && aRhs) = default;

    /// \brief Number of distinct entities matching the query.
    std::size_t countMatches() const;

    /// \brief Check the consistency of the handles and archetypes.
    [[nodiscard]] bool verifyArchetypes();

    /// \brief Iteration over all entities matching the query.
    template <class F_function>
    void each(F_function && aCallback);

    template <class F_function>
    void eachPair(F_function && aCallback);

    // TODO Ad 2022/07/13: Should the query notify of the potential existence of entities
    // at the moment a EntityAdded listener is installed?
    // It was decided not to do it at the moment, revising the decision if the need arises.
    // An issue would be when a listener does mutating operations such as adding/removing
    // entities or components, which phase should be used?
    template <class F_function>
    void onAddEntity(F_function && aCallback);

    template <class F_function>
    void onRemoveEntity(F_function && aCallback);

private:
    void swap(Query & aRhs);

    std::tuple<Storage<VT_components> & ...>
    getStorages(const typename detail::QueryBackend<VT_components...>::MatchedArchetype & aMatch)
    {
        return std::tie(getArchetype(aMatch).getStorage(
            std::get<StorageIndex<VT_components>>(aMatch.mComponentIndices))...);
    }

    // TODO Ad 2022/07/27 #perf p2: Have a better "handle" mechanism, e.g. an offset in an array.
    inline static const TypeSequence gTypeSequence{getTypeSequence<VT_components...>()};

    detail::QueryBackend<VT_components...> & getBackend()
    { return *mManager->queryBackend<VT_components...>(gTypeSequence); }

    const detail::QueryBackend<VT_components...> & getBackend() const
    { return *mManager->queryBackend<VT_components...>(gTypeSequence); }

    using Matched_t = typename detail::QueryBackend<VT_components...>::MatchedArchetype;
    const std::vector<Matched_t> & matches() const
    { return getBackend().mMatchingArchetypes; }

    Archetype & getArchetype(const Matched_t & aMatch)
    { return mManager->archetype(aMatch.mArchetype); }

    const Archetype & getArchetype(const Matched_t & aMatch) const
    { return mManager->archetype(aMatch.mArchetype); }

    std::vector<detail::Listening> mActiveListenings;
    EntityManager * mManager;
};


//
// Implementations
//
template <class... VT_components>
Query<VT_components...>::Query(EntityManager & aManager) :
    mManager{&aManager}
{
    // Ensure the query backend exists in the map.
    aManager.getQueryBackend<VT_components...>();
}


template <class... VT_components>
Query<VT_components...>::Query(const Query & aRhs) :
    mManager{aRhs.mManager}
{
    // Redirect all listeners to stop listening in the backends of the current backend store.
    for (const auto & listening : aRhs.mActiveListenings)
    {
        mActiveListenings.emplace_back(listening, &getBackend());
    }
}


template <class... VT_components>
Query<VT_components...> & Query<VT_components...>::operator=(const Query & aRhs)
{
    Query copy{aRhs};
    swap(copy);
    return *this;
}


template <class... VT_components>
void Query<VT_components...>::swap(Query & aRhs)
{
    std::swap(mActiveListenings, aRhs.mActiveListenings);
    std::swap(mManager, aRhs.mManager);
}


template <class... VT_components>
std::size_t Query<VT_components...>::countMatches() const
{
    return std::accumulate(matches().begin(), matches().end(),
                           std::size_t{0},
                           [this](std::size_t accu, const auto & matched)
                           {
                                // note: explicitly use this, as Clang was wrongly warning that
                                // this is unused after its capture...
                                return accu + this->getArchetype(matched).countEntities();
                           });
}


template<class... VT_components>
bool Query<VT_components...>::verifyArchetypes()
{
    const auto queryTypeSet = getTypeSet<VT_components...>();
    for(const auto & match : matches())
    {
        auto & archetype = getArchetype(match);
        bool storesConsistency = archetype.verifyStoresConsistency();
        // Checks that the handle archetype is the same 
        // as the archetype that back link to the handle
        // This is important because archetypes contain a list of
        // EntityKey that might be wrong
        bool handlesConsistency = archetype.verifyHandlesConsistency(*mManager);

        // Checks that the archetype of this match
        // contains storage for each component type in this Query.
        bool archetypeHaveRequiredTypes = true;
        for(auto componentId : queryTypeSet)
        {
            archetypeHaveRequiredTypes &= archetype.getTypeSet().contains(componentId);
        }

        if(!storesConsistency || ! handlesConsistency || !archetypeHaveRequiredTypes)
        {
            // Leave a line for the breakpoint
            return false;
        }
    }

    return true;
}


// TODO replace F_function with T_function
//
template <class... VT_components>
template <class F_function>
void Query<VT_components...>::each(F_function && aCallback)
{
#if defined(ENTITY_SANITIZE)
    assert(verifyArchetypes());
#endif
    for(const auto & match : matches())
    {
#if defined(ENTITY_SANITIZE)
        // We increment the atomic counter to signal this archetype is currently iterated.
        // This value should be checked by each operation mutating the archetype list of entities.
        auto & iterations = getArchetype(match).mCurrentQueryIterations;
        ++iterations;
        Guard iterationIncrementScope{[&iterations]{--iterations;}};
#endif
        // Note: The reference remains valid for the loop, because all operations
        // which could potentially invalidate it (such as adding a new archetype)
        // are deferred until the end of the phase.
        std::size_t size = getArchetype(match).countEntities();
        std::tuple<Storage<VT_components> & ...> storages = getStorages(match);
        const std::vector<HandleKey<Entity>> & handleKeys = getArchetype(match).getEntityIndices();
        for(std::size_t entityId = 0; entityId != size; ++entityId)
        {
            detail::Invoker<handy::FunctionArgument_tuple<F_function>>::template invoke<VT_components...>(
                std::forward<F_function>(aCallback),
                Handle<Entity>{handleKeys[entityId], *mManager},
                storages,
                entityId);
        }
    }
}


template <class... VT_components>
template <class F_function>
void Query<VT_components...>::eachPair(F_function && aCallback)
{
#if defined(ENTITY_SANITIZE)
    assert(verifyArchetypes());
#endif
    for(auto matchItA = matches().begin();
        matchItA != matches().end();
        ++matchItA)
    {
#if defined(ENTITY_SANITIZE)
        auto & iterations = getArchetype(*matchItA).mCurrentQueryIterations;
        ++iterations;
        Guard iterationIncrementScope{[&iterations]{--iterations;}};
#endif
        // Note: The reference remains valid for the loop, because all operations
        // which could potentially invalidate it (such as adding a new archetype)
        // are deferred until the end of the phase.
        Archetype & archetypeA = getArchetype(*matchItA);
        std::tuple<Storage<VT_components> & ...> storagesA = getStorages(*matchItA);
        const std::vector<HandleKey<Entity>> & handleKeysA = getArchetype(*matchItA).getEntityIndices();
        for(std::size_t entityIdA = 0;
            entityIdA != archetypeA.countEntities();
            ++entityIdA)
        {
            // remaining entities in current archetype
            for(std::size_t entityIdB = entityIdA + 1;
                entityIdB != archetypeA.countEntities();
                ++entityIdB)
            {
                detail::InvokerPair<handy::FunctionArgument_tuple<F_function>>::template invoke<VT_components...>(
                        std::forward<F_function>(aCallback),
                        Handle<Entity>{handleKeysA[entityIdA], *mManager},
                        storagesA,
                        entityIdA,
                        Handle<Entity>{handleKeysA[entityIdB], *mManager},
                        storagesA,
                        entityIdB);
            }

            // remaining archetypes
            for(auto matchItB = matchItA + 1;
                matchItB != matches().end();
                ++matchItB)
            {
#if defined(ENTITY_SANITIZE)
                auto & iterations = getArchetype(*matchItB).mCurrentQueryIterations;
                ++iterations;
                Guard iterationIncrementScope{[&iterations]{--iterations;}};
#endif
                Archetype & archetypeB = getArchetype(*matchItB);
                std::tuple<Storage<VT_components> & ...> storagesB = getStorages(*matchItB);
                const std::vector<HandleKey<Entity>> & handleKeysB = getArchetype(*matchItB).getEntityIndices();
                for(std::size_t entityIdB = 0;
                    entityIdB != archetypeB.countEntities();
                    ++entityIdB)
                {
                    detail::InvokerPair<handy::FunctionArgument_tuple<F_function>>::template invoke<VT_components...>(
                            std::forward<F_function>(aCallback),
                            Handle<Entity>{handleKeysA[entityIdA], *mManager},
                            storagesA,
                            entityIdA,
                            Handle<Entity>{handleKeysB[entityIdB], *mManager},
                            storagesB,
                            entityIdB);
                }
            }
        }
    }
}


template <class... VT_components>
template <class F_function>
void Query<VT_components...>::onAddEntity(F_function && aCallback)
{
    mActiveListenings.push_back(
        getBackend().listenEntityAdded(std::forward<F_function>(aCallback)));
}


template <class... VT_components>
template <class F_function>
void Query<VT_components...>::onRemoveEntity(F_function && aCallback)
{
    mActiveListenings.push_back(
        getBackend().listenEntityRemoved(std::forward<F_function>(aCallback)));
}


} // namespace ent
} // namespace ad

#pragma once

#include "EntityManager.h"

#include "detail/QueryBackend.h"
#include "entity/Component.h"

#include <numeric>


namespace ad {
namespace ent {

// Implementation note: this class shares the QueryBackend with all its instantiation
// (with matching components's types) for the same EntityManager.
/// \brief This is the class used by client code to query entities with corresponding components.
template <class... VT_components>
class Query
{
public:
    /// \brief Instantiate the query for the provided EntityManager.
    ///
    /// Caching of queries will be handled automatically.
    Query(EntityManager & aManager);

    Query(const Query & aRhs);
    Query & operator=(const Query & aRhs);

    Query(Query && aRhs) = default;
    Query & operator=(Query && aRhs) = default;

    /// \brief Number of distinct entities matching the query.
    std::size_t countMatches() const;

    /// \brief Iteration over all entities matching the query.
    template <class F_function>
    void each(F_function && aCallback);

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
    mActiveListenings{aRhs.mActiveListenings},
    mManager{aRhs.mManager}
{
    // Redirect all listeners to stop listening in the backends of the current backend store.
    for (auto & listening : mActiveListenings)
    {
        listening.redirect(&getBackend());
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
                           0,
                           [this](std::size_t accu, const auto & matched)
                           {
                                return accu + getArchetype(matched).countEntities();
                           });
}


template <class... VT_components>
template <class F_function>
void Query<VT_components...>::each(F_function && aCallback)
{
    for(const auto & match : matches())
    {
        // Note: The reference remains valid for the loop, because all operations
        // which could potentially invalidate it (such as adding a new archetype)
        // are deferred until the end of the phase.
        Archetype & archetype = getArchetype(match);
        for(std::size_t entityId = 0; entityId != archetype.countEntities(); ++entityId)
        {
            detail::invoke<VT_components...>(
                    std::forward<F_function>(aCallback),
                    archetype,
                    match,
                    entityId);
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

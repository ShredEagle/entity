#pragma once

#include "detail/QueryBackend-decl.h"

#include <cstddef>

namespace ad {
namespace ent {

template <class T_component>
class Storage;

class EntityManager;

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
    getStorages(const typename detail::QueryBackend<VT_components...>::MatchedArchetype & aMatch);

    // TODO Ad 2022/07/27 #perf p2: Have a better "handle" mechanism, e.g. an offset in an array.
    inline static const TypeSequence gTypeSequence{getTypeSequence<VT_components...>()};

    detail::QueryBackend<VT_components...> & getBackend();

    const detail::QueryBackend<VT_components...> & getBackend() const;

    using Matched_t = typename detail::QueryBackend<VT_components...>::MatchedArchetype;
    const std::vector<Matched_t> & matches() const;

    Archetype & getArchetype(const Matched_t & aMatch);

    const Archetype & getArchetype(const Matched_t & aMatch) const;

    std::vector<detail::Listening> mActiveListenings;
    EntityManager * mManager;
};

}
}

#pragma once

#include "EntityManager.h"

#include "detail/QueryBackend.h"

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

    /// \brief Number of distinct entities matching the query.
    std::size_t countMatches() const;

    /// \brief Iteration over all entities matching the query.
    template <class F_function>
    void each(F_function aCallback);

    template <class F_function>
    void onAddEntity(F_function && aCallback);

private:
    const std::vector<
        typename detail::QueryBackend<VT_components...>::MatchedArchetype> & matches() const
    { return mSharedBackend->mMatchingArchetypes; }

    detail::QueryBackend<VT_components...>* mSharedBackend;
};


//
// Implementations
//
template <class... VT_components>
Query<VT_components...>::Query(EntityManager & aManager) :
    mSharedBackend{aManager.getQueryBackend<VT_components...>()}
{}


template <class... VT_components>
std::size_t Query<VT_components...>::countMatches() const
{
    return std::accumulate(matches().begin(), matches().end(),
                           0, [](std::size_t accu, const auto & matched)
                           {
                                return accu + matched.mArchetype->countEntities();
                           });
}


template <class... VT_components>
template <class F_function>
void Query<VT_components...>::each(F_function aCallback)
{
    for(const auto & match : matches())
    {
        for(std::size_t entityId = 0; entityId != match.mArchetype->countEntities(); ++entityId)
        {
            aCallback(std::get<VT_components *>(match.mComponents)[entityId]...);
        }
    }
}


template <class... VT_components>
template <class F_function>
void Query<VT_components...>::onAddEntity(F_function && aCallback)
{
    mSharedBackend->mAddListeners.push_back(std::forward<F_function>(aCallback));
}


} // namespace ent
} // namespace ad

#pragma once

#include "Archetype.h"
#include "EntityManager.h"

#include <numeric>


namespace ad {
namespace ent {


// TODO implement a cache system for queries
// TODO queries should get notified of new archetypes, to test for inclusion.
template <class... VT_components>
class Query
{
public:
    Query(EntityManager & aManager);

    /// \brief Number of distinct entities matching the query.
    std::size_t countMatches() const;

private:
    std::vector<Archetype *> mMatchingArchetypes;
};


//
// Implementations
//
template <class... VT_components>
Query<VT_components...>::Query(EntityManager & aManager)
{
    for(auto & [typeSet, archetype] : aManager.mArchetypes)
    {
        const TypeSet queryTypeSet = getTypeSet<VT_components...>();
        if(std::includes(typeSet.begin(), typeSet.end(),
                         queryTypeSet.begin(), queryTypeSet.end()))
        {
            mMatchingArchetypes.push_back(&archetype);
        }
    }
}


template <class... VT_components>
std::size_t Query<VT_components...>::countMatches() const
{
    return std::accumulate(mMatchingArchetypes.begin(), mMatchingArchetypes.end(),
                           0, [](std::size_t accu, const Archetype * archetype)
                           {
                                return accu + archetype->countEntities();
                           });
}


} // namespace ent
} // namespace ad

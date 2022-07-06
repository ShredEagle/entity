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
    struct MatchedArchetype
    {
        MatchedArchetype(Archetype * aArchetype);

        Archetype * mArchetype;
        std::tuple<VT_components *...> mComponents;
    };

public:
    Query(EntityManager & aManager);

    /// \brief Number of distinct entities matching the query.
    std::size_t countMatches() const;

    template <class F_function>
    void each(F_function aCallback);

private:
    std::vector<MatchedArchetype> mMatchingArchetypes;
};


//
// Implementations
//
template <class... VT_components>
Query<VT_components...>::MatchedArchetype::MatchedArchetype(Archetype * aArchetype) :
    mArchetype{aArchetype},
    mComponents{mArchetype->begin<VT_components>()...}
{}


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
                           0, [](std::size_t accu, const MatchedArchetype matched)
                           {
                                return accu + matched.mArchetype->countEntities();
                           });
}


template <class... VT_components>
template <class F_function>
void Query<VT_components...>::each(F_function aCallback)
{
    for(const auto & match : mMatchingArchetypes)
    {
        for(std::size_t entityId = 0; entityId != match.mArchetype->countEntities(); ++entityId)
        {
            aCallback(std::get<VT_components *>(match.mComponents)[entityId]...);
        }
    }
}


} // namespace ent
} // namespace ad

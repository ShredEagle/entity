#include "EntityManager.h"

#include <iterator>


namespace ad {
namespace ent {


Archetype & EntityManager::getArchetype(const TypeSet & aTypeSet)
{
    return mArchetypes.at(aTypeSet);
}


std::size_t EntityManager::countLiveEntities() const
{
    assert(mHandleMap.size() >= mFreedHandles.size());

    return mHandleMap.size() - mFreedHandles.size();
}


EntityRecord & EntityManager::record(HandleKey aKey)
{
    // TODO implement checks:
    // * that the generation is matching
    // * that the record is still there? (or is it a responsibility of the client to check for nullptr?)
    return mHandleMap.at(aKey);
}


void EntityManager::freeHandle(HandleKey aKey)
{
    record(aKey).mArchetype = nullptr;
    mFreedHandles.push_back(std::move(aKey));
}


std::vector<detail::QueryBackendBase *>
EntityManager::getExtraQueryBackends(const Archetype & aCompared,
                                     const Archetype & aReference) const
{
    using QueryBackendSet = std::set<detail::QueryBackendBase *>;

    auto getQueries = [&](const Archetype & aArchetype)
    {
        QueryBackendSet result;
        TypeSet archetypeTypeSet = aArchetype.getTypeSet();
        for(const auto & [typeSequence, backend] : mQueryBackends)
        {
            TypeSet queryTypeSet{typeSequence.begin(), typeSequence.end()};
            if(std::includes(archetypeTypeSet.begin(), archetypeTypeSet.end(),
                             queryTypeSet.begin(), queryTypeSet.end()))
            {
                result.insert(backend.get());
            }
        }
        return result;
    };

    QueryBackendSet initialQueries = getQueries(aReference);
    QueryBackendSet targetQueries = getQueries(aCompared);
    std::vector<detail::QueryBackendBase *> difference;
    std::set_difference(targetQueries.begin(), targetQueries.end(),
                        initialQueries.begin(), initialQueries.end(),
                        std::back_inserter(difference));
    return difference;
}


} // namespace ent
} // namespace ad

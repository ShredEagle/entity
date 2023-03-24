#include "EntityManager.h"
#include "entity/Component.h"

#include <iterator>


namespace ad {
namespace ent {


Handle<Archetype> EntityManager::InternalState::getArchetypeHandle(const TypeSet & aTypeSet,
                                                                   EntityManager & aManager)
{
    return {mArchetypes.getKey(aTypeSet), aManager};
}


std::size_t EntityManager::InternalState::countLiveEntities() const
{
    assert(mHandleMap.size() >= mFreedHandles.size());

    return mHandleMap.size() - mFreedHandles.size();
}


EntityRecord & EntityManager::InternalState::record(HandleKey<Entity> aKey)
{
    // TODO implement checks:
    // * that the generation is matching
    // * that the record is still there? (or is it a responsibility of the client to check for nullptr?)
    return mHandleMap.at(aKey);
}


Archetype & EntityManager::InternalState::archetype(HandleKey<Archetype> aHandle)
{
    return mArchetypes.get(aHandle);
}


void EntityManager::InternalState::freeHandle(HandleKey<Entity> aKey)
{
    record(aKey).mIndex = gInvalidIndex;
    mFreedHandles.push_back(std::move(aKey));
}

std::set<detail::QueryBackendBase *>
EntityManager::InternalState::getQueryBackendSet(const Archetype & aArchetype) const
{
    std::set<detail::QueryBackendBase *> result;
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
}


std::vector<detail::QueryBackendBase *>
EntityManager::InternalState::getExtraQueryBackends(const Archetype & aCompared,
                                     const Archetype & aReference) const
{
    using QueryBackendSet = std::set<detail::QueryBackendBase *>;

    QueryBackendSet initialQueries = getQueryBackendSet(aReference);
    QueryBackendSet targetQueries = getQueryBackendSet(aCompared);
    std::vector<detail::QueryBackendBase *> difference;
    std::set_difference(targetQueries.begin(), targetQueries.end(),
                        initialQueries.begin(), initialQueries.end(),
                        std::back_inserter(difference));
    return difference;
}


HandleKey<Entity> EntityManager::InternalState::getAvailableHandle()
{
    if (mFreedHandles.empty())
    {
        return mNextHandle++;
    }
    else
    {
        HandleKey<Entity> handle = mFreedHandles.front();
        mFreedHandles.pop_front();
        return handle;
    }
}


State EntityManager::saveState()
{
    // move the currently active InternalState to the backup State.
    State backup{std::move(mState)};
    // Default intialize a new state in the EntityManager.
    // From this point, all accesses via handles will resolve into this new state.
    mState = std::make_unique<InternalState>();
    // Copy-assign the saved state into the defaulted new active state.
    *mState = *backup.mState;

    return backup;
}


void EntityManager::restoreState(const State & aState)
{
    assert(aState.mState != nullptr); // the default constructed
    mState = std::make_unique<InternalState>();
    *mState = *aState.mState;
}


} // namespace ent
} // namespace ad

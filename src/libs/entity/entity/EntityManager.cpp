#include "EntityManager.h"

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


} // namespace ent
} // namespace ad

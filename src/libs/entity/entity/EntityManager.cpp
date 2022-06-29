#include "EntityManager.h"

namespace ad {
namespace ent {


Archetype & EntityManager::getArchetype(const TypeSet & aTypeSet)
{
    return mArchetypes.at(aTypeSet);
}


std::size_t EntityManager::countLiveEntities() const
{
    return mHandleMap.size();
}

} // namespace ent
} // namespace ad

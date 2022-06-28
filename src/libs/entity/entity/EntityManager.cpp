#include "EntityManager.h"

namespace ad {
namespace ent {


EntityRecord Entity::record() const
{
    return mManager.mHandleMap.at(mKey);
}


void Entity::updateRecord(EntityRecord aNewRecord)
{
    mManager.mHandleMap.at(mKey) = aNewRecord;
}


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

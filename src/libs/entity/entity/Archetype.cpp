#include "Archetype.h"
#include "Component.h"
#include "EntityManager.h"


namespace ad {
namespace ent {


std::size_t Archetype::countEntities() const
{
    auto result = mHandles.size();

    // TODO implement level of assertion / way to disable.
    // All stores **have to** be of the same size.
    for(auto storeIt = mStores.begin(); storeIt != mStores.end(); ++storeIt)
    {
        auto s = (*storeIt)->size();
        assert(s == result);
    }

    return result;
}


bool Archetype::verifyConsistency()
{
    if(mType.size() != mStores.size())
    {
        return false;
    }

    std::size_t entitiesCount = countEntities();

    if(entitiesCount != mHandles.size())
    {
        return false;
    }

    for(std::size_t storeId = 0; storeId != mType.size(); ++storeId)
    {
        if(mType[storeId] != mStores[storeId]->getType())
        {
            return false;
        }
        else if(mStores[storeId]->size() != entitiesCount)
        {
            return false;
        }
    }

    return true;
}


void Archetype::move(std::size_t aEntityIndex, Archetype & aDestination, EntityManager & aManager)
{
    // Move the matching components from the stores of `this` archetype to the destination stores.
    for(std::size_t sourceStoreId = 0; sourceStoreId != mType.size(); ++sourceStoreId)
    {
        for(std::size_t destinationStoreId = 0; destinationStoreId != aDestination.mType.size(); ++destinationStoreId)
        {
            // Found matching components, move-push from source at the back of destination
            if(mType[sourceStoreId] == aDestination.mType[destinationStoreId])
            {
                aDestination.mStores[destinationStoreId]->moveFrom(aEntityIndex, *mStores[sourceStoreId]);
            }
        }
    }
    // Copy the HandleKey for the moved entity.
    aDestination.mHandles.push_back(mHandles[aEntityIndex]);

    remove(aEntityIndex, aManager);
}


void Archetype::remove(EntityIndex aEntityIndex, EntityManager & aManager)
{
    // Redirects the handle of the entity that will take the place of the removed entity
    {
        // This is the entity that will take the place of the removed entity in the stores.
        HandleKey replacementEntity = mHandles.back();
        // Overwrite with the same value if the replacement entity is the one at aEntityIndex
        // (i.e., if it was the last entity in the archetype).
        aManager.record(replacementEntity).mIndex = aEntityIndex;
    }

    // Erase the handle.
    detail::eraseByMoveOver(mHandles, aEntityIndex);

    // Erase the components.
    for(std::size_t storeId = 0; storeId != mType.size(); ++storeId)
    {
        mStores[storeId]->remove(aEntityIndex);
    }
}


} // namespace ent
} // namespace ad

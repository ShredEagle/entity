#include "Archetype.h"
#include "entity/Component.h"


namespace ad {
namespace ent {


std::size_t Archetype::countEntities() const
{
    assert(mType.size() > 0);

    // All stores **have to** be of the same size.
    auto result = mStores.front()->size();

    // TODO implement level of assertion / way to disable.
    // Assert all stores are indeed of the same size.
    for(auto storeIt = mStores.begin() + 1; storeIt != mStores.end(); ++storeIt)
    {
        assert((*storeIt)->size() == result);
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


void Archetype::move(std::size_t aEntityIndex, Archetype & aDestination)
{
    for(std::size_t sourceStoreId = 0; sourceStoreId != mType.size(); ++sourceStoreId)
    {
        for(std::size_t destinationStoreId = 0; destinationStoreId != mType.size(); ++destinationStoreId)
        {
            // Found matching components, move from source into destination
            if(mType[sourceStoreId] == aDestination.mType[destinationStoreId])
            {
                aDestination.mStores[destinationStoreId]->moveFrom(aEntityIndex, *mStores[sourceStoreId]);
            }
        }
    }

    remove(aEntityIndex);
}


void Archetype::remove(EntityIndex aEntityIndex)
{
    for(std::size_t storeId = 0; storeId != mType.size(); ++storeId)
    {
        mStores[storeId]->remove(aEntityIndex);
    }
}


} // namespace ent
} // namespace ad

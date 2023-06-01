#include "Archetype.h"
#include "Component.h"
#include "EntityManager.h"


namespace ad {
namespace ent {


DataStore::DataStore(const DataStore & aRhs)
{
    for(const auto & storagePtr : aRhs)
    {
        push_back(storagePtr->clone());
    }
}


DataStore & DataStore::operator=(const DataStore & aRhs)
{
    DataStore copy{aRhs};
    swap(copy);
    return *this;
}


void DataStore::swap(DataStore & aRhs)
{
    std::swap(static_cast<Parent_t &>(*this),
              static_cast<Parent_t &>(aRhs));
}


std::size_t Archetype::countEntities() const
{
    auto result = mHandles.size();

#if defined(ENTITY_SANITIZE)
    assert(checkStoreSize());
#endif

    return result;
}


bool Archetype::checkStoreSize() const
{
    bool result = true;
    for(auto storeIt = mStores.begin(); storeIt != mStores.end(); ++storeIt)
    {
        auto s = (*storeIt)->size();
        result &= s == mHandles.size();
    }
    return result;
}


bool Archetype::verifyHandlesConsistency(EntityManager & aManager)
{
    for(std::size_t entityIdx = 0; entityIdx != mHandles.size(); ++entityIdx)
    {
        Handle<Entity> handle{mHandles[entityIdx], aManager};
        // True if index `mIndex` stored in the EntityRecord is the same as the actual index of the Entity in the archetype.
        bool orderIsConsistent = handle.record().mIndex == entityIdx;
        // True if the archetype for the entity behind handleKey is `this` instance.
        bool archetypeIsConsistent = &handle.archetype() == this;
        if (!orderIsConsistent || !archetypeIsConsistent)
        {
            // Leave a line for the breakpoint
            return false;
        }
    }
    return true;
}


bool Archetype::verifyStoresConsistency()
{
    // Ensure there are as much Stores for components as types in the the Archetype.
    if(mType.size() != mStores.size())
    {
        return false;
    }

    // Ensure that the type of component in each Store matche the type in mType.
    for(std::size_t storeId = 0; storeId != mType.size(); ++storeId)
    {
        if(mType[storeId] != mStores[storeId]->getType())
        {
            return false;
        }
    }

    // Ensure the entities counts matches the count of handles (redundant) and the number of element in each Storage.
    std::size_t entitiesCount = countEntities();
    if(entitiesCount != mHandles.size() || !checkStoreSize())
    {
        return false;
    }

    return true;
}


void Archetype::move(std::size_t aEntityIndex, Archetype & aDestination, EntityManager & aManager)
{
#if defined(ENTITY_SANITIZE)
    // If one of the archetypes is currently under iteration via Query::each(),
    // the iterated containers would be modified during the iteration, which is an error.
    assert(this->mCurrentQueryIterations == 0);
    assert(aDestination.mCurrentQueryIterations == 0);
#endif

    // Move the matching components from the stores of `this` archetype to the destination stores.
    for(std::size_t sourceStoreId = 0;
        sourceStoreId != mType.size();
        ++sourceStoreId)
    {
        for(std::size_t destinationStoreId = 0; 
            destinationStoreId != aDestination.mType.size();
            ++destinationStoreId)
        {
            // Found matching components, move-push from source at the back of destination
            if(mType[sourceStoreId] == aDestination.mType[destinationStoreId])
            {
                aDestination.mStores[destinationStoreId]
                    ->moveFrom(aEntityIndex, *mStores[sourceStoreId]);
                break; // Once the matching destination store has been found, go to next source store.
            }
        }
    }
    // Copy the HandleKey for the moved entity.
    aDestination.mHandles.push_back(mHandles[aEntityIndex]);

    remove(aEntityIndex, aManager);
}


void Archetype::remove(EntityIndex aEntityIndex, EntityManager & aManager)
{
#if defined(ENTITY_SANITIZE)
    // If this archetype is currently under iteration via Query::each(), there is an error.
    assert(mCurrentQueryIterations == 0);
#endif

    // Redirects the handle of the entity that will take the place of the removed entity
    {
        // This is the entity that will take the place of the removed entity in the stores.
        HandleKey<Entity> replacementEntity = mHandles.back();
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


void Archetype::pushKey(HandleKey<Entity> aKey)
{
#if defined(ENTITY_SANITIZE)
    // If this archetype is currently under iteration via Query::each(), there is an error.
    assert(mCurrentQueryIterations == 0);
#endif
    mHandles.push_back(aKey); 
}


} // namespace ent
} // namespace ad

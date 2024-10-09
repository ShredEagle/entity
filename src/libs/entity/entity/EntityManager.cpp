#include "EntityManager.h"

#include "Component.h"
#include "Archetype.h"
#include "Blueprint.h"

#include <iterator>
#include <cassert>


namespace ad {
namespace ent {

Handle<Entity> EntityManager::createFromBlueprint(Handle<Entity> aBlueprint, const char * aName)
{
    assert(aBlueprint.isValid());
    Phase blueprint;

    auto newHandle = addEntity(aName);

    aBlueprint.get(blueprint)->copy(newHandle);
    newHandle.get(blueprint)->remove<Blueprint>();

    return newHandle;
}

EntityManager & EntityManager::getEmptyHandleEntityManager()
{
    static EntityManager emptyHandleEntityManager;
    // Note: Add an entry for the invalid entity key, mapping it to an invalid EntityRecord.
    // This design allows to implement Handle<Entity>::isValid() logic 
    // without explicitly testing for the invalid key.
    emptyHandleEntityManager.mState->insertInvalidHandleKey();
    return emptyHandleEntityManager;
}


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


const HandleKey<Entity> & EntityManager::InternalState::keyForIndex(HandleKey<Entity> aKey)
{
    return mHandleMap.find(aKey)->first;
}


Archetype & EntityManager::InternalState::archetype(HandleKey<Archetype> aHandle)
{
    return mArchetypes.get(aHandle);
}

const char * EntityManager::InternalState::name(HandleKey<Entity> aHandle) const
{
    return mHandleMap.at(aHandle).mNamePtr->c_str();
}


void EntityManager::InternalState::freeHandle(HandleKey<Entity> aKey)
{ 
    // Even though it is possible that the latest handle key is use for a legitimate Handle,
    // it is very unlikely. 
    // On the other hand, it is used for the default constructed Handle, and it would be an error
    // to be able to free it.
    assert(aKey != HandleKey<Entity>::MakeLatest());

    // Get the reference to the currently stored handle key.
    const auto & handleKey = keyForIndex(aKey);
    // Increment the generation of the key in the map, so any existing handle to the freed element
    // will not compare equal anymore (and thus will not be considered to point to a valid Entity).
    handleKey.advanceGeneration();

    // Important: the handle with advanced generation is stord in the free list
    // because this is the handle that will be returned by an `addEntity()` re-using it.
    mFreedHandles.push_back(handleKey);
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


void EntityManager::InternalState::insertInvalidHandleKey()
{
    // The default constructed Handle (which is not valid) use the latest HandleKey value.
    // Storing a record at the same Handle, except its generation wrapped around,
    // ensure that the default Handle is not valid.
    mHandleMap.emplace(
        HandleKey<Entity>::MakeLatest().advanceGeneration(),
        EntityRecord{HandleKey<Archetype>::MakeLatest(), // does not matter
                     std::numeric_limits<EntityIndex>::max()}
    );
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

void EntityManager::forEachHandle(std::function<void(Handle<Entity>, const char *)> aCallback)
{
    mState->forEachHandle(std::move(aCallback), *this);
}

void EntityManager::InternalState::forEachHandle(std::function<void(Handle<Entity>, const char *)> aCallback, EntityManager & aManager)
{
    for (auto & [key, record] : mHandleMap)
    {
        Handle<Entity> handle(key, aManager);
        if (std::find(mFreedHandles.begin(), mFreedHandles.end(), key) == mFreedHandles.end())
        {
            aCallback(handle, record.mNamePtr->c_str());
        }
    }
}


} // namespace ent
} // namespace ad

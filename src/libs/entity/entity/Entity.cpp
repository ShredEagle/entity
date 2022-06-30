#include "Entity.h"

#include "EntityManager.h"

namespace ad {
namespace ent {


Phase::~Phase()
{
    for (auto & operation : mOperations)
    {
        operation();
    }
}


void Entity::erase()
{
    mPhase.append(
        [&handle = mHandle]
        {
            handle.erase();
        });
}


std::optional<Entity> Handle<Entity>::get(Phase & aPhase)
{
    EntityRecord current = record();

    // TODO Potential optimisation: is it wise to to the check here?
    // Knowing that the client has to check.
    // record() already return a nullptr archetype for invalid entities,
    // which could directly be checked by the client.
    if(current.mArchetype != nullptr)
    {
        return Entity{
            record(),
            *this,
            aPhase,
        };
    }
    else
    {
        return std::nullopt;
    }
}


void Handle<Entity>::erase()
{
    EntityRecord rec = record();
    rec.mArchetype->remove(rec.mIndex);

    rec.mArchetype = nullptr;
    updateRecord(rec);
}


EntityRecord Handle<Entity>::record() const
{
    // TODO implement checks that the record is still there
    // that the generation is matching
    return mManager.mHandleMap.at(mKey);
}


void Handle<Entity>::updateRecord(EntityRecord aNewRecord)
{
    mManager.mHandleMap.at(mKey) = aNewRecord;
}


} // namespace ent
} // namespace ad

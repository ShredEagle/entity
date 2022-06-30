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


bool Handle<Entity>::isValid() const
{
    EntityRecord current = record();
    return current.mArchetype != nullptr;
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
    rec.mArchetype->remove(rec.mIndex, mManager);

    mManager.freeHandle(mKey);
}


EntityRecord Handle<Entity>::record() const
{
    return mManager.record(mKey);
}


void Handle<Entity>::updateRecord(EntityRecord aNewRecord)
{
    // Must mutate the underlying record, so it cannot use this->record().
    mManager.record(mKey) = aNewRecord;
}


} // namespace ent
} // namespace ad

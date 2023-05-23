#include "Entity.h"

#include "EntityManager.h"
#include "entity/Component.h"

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
        [handle = mHandle] () mutable
        {
            handle.erase();
        });
}


Archetype & Handle<Archetype>::get()
{
    return mManager->archetype(mKey);
}


bool Handle<Entity>::isValid() const
{
    EntityRecord current = record();
    return current.mIndex != gInvalidIndex;
}


std::optional<Entity_view> Handle<Entity>::get() const
{
    EntityRecord current = record();

    // TODO Potential optimisation: is it wise to to the check here?
    // Knowing that the client has to check.
    // record() already return a nullptr archetype for invalid entities,
    // which could directly be checked by the client.
    if(current.mIndex != gInvalidIndex)
    {
        return Entity_view{
            reference(),
        };
    }
    else
    {
        return std::nullopt;
    }
}


std::optional<Entity> Handle<Entity>::get(Phase & aPhase)
{
    EntityRecord current = record();

    // TODO Potential optimisation: is it wise to to the check here?
    // Knowing that the client has to check.
    // record() already return a nullptr archetype for invalid entities,
    // which could directly be checked by the client.
    if(current.mIndex != gInvalidIndex)
    {
        return Entity{
            reference(),
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
    Archetype & arch = archetype();
    auto querySet = mManager->getQueryBackendSet(arch);

    for (const auto & query : querySet)
    {
        query->signalEntityRemoved(*this, rec);
    }

    arch.remove(rec.mIndex, *mManager);

    mManager->freeHandle(mKey);
}


// TODO redesign this group of retrieval functions
// This approach tends to call record() several times...
EntityRecord Handle<Entity>::record() const
{
    return mManager->record(mKey);
}

Archetype & Handle<Entity>::archetype() const
{
    return mManager->archetype(record().mArchetype);
}


EntityReference Handle<Entity>::reference() const
{
    return {
        &archetype(),
        record().mIndex,
    };
}


void Handle<Entity>::updateRecord(EntityRecord aNewRecord)
{
    // Must mutate the underlying record, so it cannot use this->record().
    mManager->record(mKey) = aNewRecord;
}


} // namespace ent
} // namespace ad

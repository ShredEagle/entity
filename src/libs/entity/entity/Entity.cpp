#include "Entity.h"

#include "EntityManager.h"

namespace ad {
namespace ent {


EntityRecord Handle<Entity>::record() const
{
    // TODO implement checks that the record is still there
    // that the generation is matching
    return mManager.mHandleMap.at(mKey);
}


std::optional<Entity> Handle<Entity>::get(Phase & aPhase)
{
    return Entity{
        record(),
        *this,
        aPhase,
    };
}


void Handle<Entity>::updateRecord(EntityRecord aNewRecord)
{
    mManager.mHandleMap.at(mKey) = aNewRecord;
}


} // namespace ent
} // namespace ad

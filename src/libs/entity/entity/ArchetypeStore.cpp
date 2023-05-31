#include "ArchetypeStore.h"


namespace ad {
namespace ent {


ArchetypeStore & ArchetypeStore::operator=(const ArchetypeStore & aRhs)
{
    mTypeSetToArchetype = aRhs.mTypeSetToArchetype;
    mHandleToArchetype.clear();
    for(const auto & archetypePtr : aRhs.mHandleToArchetype)
    {
        mHandleToArchetype.push_back(std::make_unique<Archetype>(*archetypePtr));
    }
    return *this;
}


} // namespace ent
} // namespace ad
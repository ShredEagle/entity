#include <entity/EntityManager.h>


namespace ad {
namespace ent {


template <class T_inspected>
class Inspector;

template <>
class Inspector<EntityManager>
{
public:
    static std::size_t countArchetypes(const EntityManager & aEntityManager)
    { return aEntityManager.mState->mArchetypes.size(); }

    template <class... VT_components>
    static Handle<Archetype> getArchetypeHandle(EntityManager & aEntityManager)
    { return aEntityManager.getArchetypeHandle(getTypeSet<VT_components...>()); }
};


} // namespace ent
} // namespace ad



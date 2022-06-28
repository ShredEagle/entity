#pragma once

#include "Component.h"

#include <vector>


namespace ad {
namespace ent {


class ArchetypeBase
{
protected:
    std::vector<ComponentId> mType;
    std::vector<void *> mStores;
};


template <class... VT_Components>
class Archetype : public ArchetypeBase
{
public:
    Archetype();

private:
    std::tuple<std::vector<VT_Components>...> mComponents;
};


template <class... VT_Components>
Archetype<VT_Components...>::Archetype()
{
    mType = std::vector<ComponentId>{
        getId<VT_Components>...
    };

    mStores = std::vector<void *>{
        reinterpret_cast<void *>(std::get<VT_Components>(mComponents).data())...
    };
}


} // namespace ent
} // namespace ad

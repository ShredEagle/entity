#pragma once


#include <set>
#include <typeindex>


namespace ad {
namespace ent {


using EntityIndex = std::size_t;


using ComponentId = std::type_index;


template <class T_component>
inline ComponentId getId()
{
    return std::type_index(typeid(T_component));
}


using TypeSet = std::set<ComponentId>;


template <class... VT_components>
TypeSet getTypeSet()
{
    return TypeSet{ {getId<VT_components>()...} };
}


} // namespace ent
} // namespace ad

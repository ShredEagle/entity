#pragma once


#include <set>
#include <typeindex>
#include <vector>


namespace ad {
namespace ent {


using EntityIndex = std::size_t;
// TODO to be removed, when the validity of an handle is to be tested based on its generation.
constexpr EntityIndex gInvalidIndex = std::numeric_limits<EntityIndex>::max();


using ComponentId = std::type_index;


template <class T_component>
inline ComponentId getId()
{
    return std::type_index(typeid(T_component));
}


// TODO A constexpr datastructure would allow some optimizations.
// (such as not storing the query type set in a static data member.)
/// \brief Ordered set of ComponentIds.
/// Being ordered, the TypeSet value does not depend on the order the components are provided.
using TypeSet = std::set<ComponentId>;

template <class... VT_components>
TypeSet getTypeSet()
{
    return TypeSet{ {getId<VT_components>()...} };
}


// TODO Ad 2022/07/08: Ideally, this types becomes unused and can be removed.
// It is first implemented because QueryBackends are re-instantiated for different orderings
// of the same component set.
/// \brief Depends on the order in which the components are provided.
using TypeSequence = std::vector<ComponentId>;

template <class... VT_components>
TypeSequence getTypeSequence()
{
    return TypeSequence{ {getId<VT_components>()...} };
}


} // namespace ent
} // namespace ad

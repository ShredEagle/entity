#pragma once


#include <entity/Entity.h>

#include <tuple>


namespace ad {
namespace ent {
namespace detail {


/// \brief Invoke a callback on a matched archetype, for a given entity in the archetype.
/// \note Implemented as a templatated static member function of templated class instead a a templated function:
/// we want to partially specialize on the callback arguments, to detect when the callback expects the handle to the entity.
template <class T_callbackArgsTuple>
struct Invoker;

// Partial specialization, when the callback does not take an Handle<Entity>
template <class... VT_callbackArgs>
struct Invoker<std::tuple<VT_callbackArgs...>>
{
    template <class... VT_components, class F_callback>
    static void invoke(F_callback aCallback,
                       std::tuple<Storage<VT_components> & ...> aStorages,
                       EntityIndex aIndexInArchetype)
    {
        // get on the callback arguments types in order to allow 
        // callback taking a subset of components / out of order components.
        aCallback(std::get<Storage<std::decay_t<VT_callbackArgs>> &>(aStorages)
                    .mArray[aIndexInArchetype]...);
    }

    template <class... VT_components, class F_callback>
    static void invoke(F_callback && aCallback,
                       Handle<Entity> aHandle,
                       std::tuple<Storage<VT_components> & ...> aStorages,
                       EntityIndex aIndexInArchetype)
    {
        invoke(std::forward<F_callback>(aCallback), aStorages, aIndexInArchetype);
    }
};


// Partial specialization, when the callback first arguments is an Handle<Entity>
template <class... VT_callbackArgs>
struct Invoker<std::tuple<Handle<Entity>, VT_callbackArgs...>>
{
    template <class... VT_components, class F_callback>
    static void invoke(F_callback aCallback,
                       Handle<Entity> aHandle,
                       std::tuple<Storage<VT_components> & ...> aStorages,
                       EntityIndex aIndexInArchetype)
    {
        aCallback(aHandle,
                  std::get<Storage<std::decay_t<VT_callbackArgs>> &>(aStorages)
                    .mArray[aIndexInArchetype]...);
    }
};


template <class... VT_components, class F_callback>
void invokePair(F_callback aCallback,
                std::tuple<Storage<VT_components> & ...> aStoragesA,
                EntityIndex aIndexInArchetypeA,
                std::tuple<Storage<VT_components> & ...> aStoragesB,
                EntityIndex aIndexInArchetypeB)
{
    aCallback(std::get<Storage<VT_components> &>(aStoragesA).mArray[aIndexInArchetypeA]...,
              std::get<Storage<VT_components> &>(aStoragesB).mArray[aIndexInArchetypeB]...);
}


} // namespace detail
} // namespace ent
} // namespace ad

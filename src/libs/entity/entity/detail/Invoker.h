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


/// \brief An invoker for pair iteration. (i.e. Cartesian product on a Query)
template <class T_callbackArgsTuple>
struct InvokerPair;

/// \brief Specialization for a callback not taking the entity handles.
/// \attention This can only match with a callback taking all the query components in order twice.
/// (As there is no delimiter between the parameters matching on A components and those matching on B components).
/// We might replace the flat list of parameters by two tuples, allowing to take subset of components.
template <class... VT_callbackArgsAll>
struct InvokerPair<std::tuple<VT_callbackArgsAll ...>>
{
    template <class... VT_components, class F_callback>
    static void invoke(F_callback aCallback,
                       Handle<Entity> aHandleA,
                       std::tuple<Storage<VT_components> & ...> aStoragesA,
                       EntityIndex aIndexInArchetypeA,
                       Handle<Entity> aHandleB,
                       std::tuple<Storage<VT_components> & ...> aStoragesB,
                       EntityIndex aIndexInArchetypeB)
    {
        aCallback(std::get<Storage<VT_components> &>(aStoragesA).mArray[aIndexInArchetypeA]...,
                  std::get<Storage<VT_components> &>(aStoragesB).mArray[aIndexInArchetypeB]...);
    }
};

// Important: This is not deducible. We wrap in std::tuple instead.
//template <class... VT_callbackArgsLeft, class... VT_callbackArgsRight>
//struct InvokerPair<std::tuple<Handle<Entity>, VT_callbackArgsLeft...,
//                              Handle<Entity>, VT_callbackArgsRight...>>

/// \brief Specialization for pair iteration with a callback taking the entities handles.
///
/// It can take different subsets of components for each entity of the pair,
/// yet the callback must receive the components via one std::tuple for each entity.
template <class... VT_callbackArgsLeft, class... VT_callbackArgsRight>
struct InvokerPair<std::tuple<Handle<Entity>, std::tuple<VT_callbackArgsLeft...>,
                              Handle<Entity>, std::tuple<VT_callbackArgsRight...>>>
{
    template <class... VT_components, class F_callback>
    static void invoke(F_callback aCallback,
                       Handle<Entity> aHandleA,
                       std::tuple<Storage<VT_components> & ...> aStoragesA,
                       EntityIndex aIndexInArchetypeA,
                       Handle<Entity> aHandleB,
                       std::tuple<Storage<VT_components> & ...> aStoragesB,
                       EntityIndex aIndexInArchetypeB)
    {
        // Note: not using std::make_tuple, because it deduces the target types by decaying the parameters type.
        aCallback(aHandleA,
                  std::tuple<VT_callbackArgsLeft...>{
                      std::get<Storage<std::decay_t<VT_callbackArgsLeft>> &>(aStoragesA)
                        .mArray[aIndexInArchetypeA]...},
                  aHandleB,  
                  std::tuple<VT_callbackArgsRight...>{
                      std::get<Storage<std::decay_t<VT_callbackArgsRight>> &>(aStoragesB)
                        .mArray[aIndexInArchetypeB]...});
    }
};


} // namespace detail
} // namespace ent
} // namespace ad

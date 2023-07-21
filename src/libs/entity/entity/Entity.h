#pragma once


#include "Archetype.h"
#include "HandleKey.h"
#include "Handle.h"

#include <functional>
#include <mutex>
#include <optional>

#include <cstddef>


namespace ad {
namespace ent {


class EntityManager; // forward

/// \brief This class allows to stack-up the operations to be deferred until the end of the phase.
class Phase
{
public:
    Phase() = default;

    // Not copyable
    Phase(const Phase &) = delete;
    Phase & operator=(const Phase &) = delete;

    ~Phase();

    /// \note Thread safe, so it can be used in presence of a job system.
    template <class F_operation>
    void append(F_operation && aOperation);

private:
    // TODO Use better concurrency mechanism, such as lightweight mutexes or a lock-free container.
    std::mutex mMutex;
    std::vector<std::function<void()>> mOperations;
};


/// @brief Mutating view of an entity.
///
/// The view gives mutating access to the entity components,
/// but it prevents removal of the entity / edition of the component list.
/// In other words, it only allows for immediate operations.
class Entity_view
{
public:
    Entity_view(EntityReference aReference) :
        mReference{aReference}
    {}

    template <class T_component>
    bool has();

    template <class T_component>
    T_component & get();

private:
    EntityReference mReference;
};


/// \brief The logic Entity, defining all available operations.
/// \note There is not persistent memory representation of an entity, it is 
/// only logically defined by the amalgamation of its different components.
class Entity : public Entity_view
{
    friend class Handle<Entity>;

public:
    /// \brief Add component T_component to the entity.
    template <class T_component>
    Entity & add(T_component aComponent);

    /// \brief Remove component T_component from the entity.
    template <class T_component>
    Entity & remove();

    /// \brief Remove the entity itself from the EntityManager.
    void erase();

private:
    Entity(
        EntityReference aReference,
        Handle<Entity> & aHandle,
        Phase & aPhase) :
        Entity_view{aReference},
        mHandle{aHandle},
        mPhase{aPhase}
    {}

    // For  deferred operations
    Handle<Entity> & mHandle;
    Phase & mPhase;
};


namespace detail
{
    template <class...>
    class QueryBackend;
}
//
// Implementations
//

template <class F_operation>
void Phase::append(F_operation && aOperation)
{
    std::lock_guard<std::mutex> lock{mMutex};
    mOperations.push_back(std::forward<F_operation>(aOperation));
}


template <class T_component>
Entity & Entity::add(T_component aComponent)
{
    mPhase.append(
        [handle = mHandle, component = std::move(aComponent)] () mutable
        {
            handle.add<T_component>(std::move(component));
        });
    return *this;
}


template <class T_component>
Entity & Entity::remove()
{
    mPhase.append(
        [handle = mHandle] () mutable
        {
            handle.remove<T_component>();
        });
    return *this;
}


template <class T_component>
bool Entity_view::has()
{
    return mReference.mArchetype->has<T_component>();
}


template <class T_component>
T_component & Entity_view::get()
{
    return mReference.mArchetype->get<T_component>(mReference.mIndex);
}


} // namespace ent
} // namespace ad

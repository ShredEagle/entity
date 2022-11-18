#pragma once


#include "Archetype.h"
#include "HandleKey.h"

#include <functional>
#include <mutex>
#include <optional>

#include <cstddef>


namespace ad {
namespace ent {


class EntityManager; // forward

template <class T_handled>
class Handle; // forward


template<>
class Handle<Archetype>
{
    friend class EntityManager; // for construction

public:
    Archetype & get();

    // Note: unused for the moment
    //friend bool operator==(const Handle & aLhs, const Handle & aRhs)
    //{
    //    assert(aLhs.mStore == aRhs.mStore);
    //    return aLhs.mKey == aRhs.mKey;
    //}

private:
    Handle(std::size_t aKey, EntityManager & aManager) :
        mKey{aKey},
        mManager{&aManager}
    {}

    HandleKey<Archetype> mKey;
    EntityManager * mManager;
};


struct EntityRecord
{
    HandleKey<Archetype> mArchetype;
    EntityIndex mIndex;
};


struct EntityReference
{
    Archetype * mArchetype;
    EntityIndex mIndex;
};


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


class Entity
{
    friend class Handle<Entity>;

public:
    template <class T_component>
    Entity & add(T_component aComponent);

    template <class T_component>
    Entity & remove();

    template <class T_component>
    bool has();

    template <class T_component>
    T_component & get();

    /// \brief Remove the entity itself from the EntityManager.
    void erase();

private:
    Entity(
        EntityReference aReference,
        Handle<Entity> & aHandle,
        Phase & aPhase) :
        mReference{aReference},
        mHandle{aHandle},
        mPhase{aPhase}
    {}

    // For immediate operations
    EntityReference mReference;

    // For  deferred operations
    Handle<Entity> & mHandle;
    Phase & mPhase;
};


template <class T_handled>
class Handle;

namespace detail
{
    template <class...>
    class QueryBackend;
}

template <>
class Handle<Entity>
{
    friend class Entity;
    friend class EntityManager;
    // TODO remove
    template <class...>
    friend class detail::QueryBackend;

public:
    /// \brief Checks whether the handle is valid, currently pointing to an Entity.
    // TODO not sure it should exist as a standalone, as it duplicates some logic from get()
    bool isValid() const;

    /// \important This handle must outlive the returned Entity.
    std::optional<Entity> get(Phase & aPhase);

    friend bool operator==(const Handle & aLhs, const Handle & aRhs)
    {
        assert (&aLhs.mManager == &aRhs.mManager);
        return aLhs.mKey == aRhs.mKey;
    }

private:
    Handle(HandleKey<Entity> aKey, EntityManager & aManager) :
        mKey{aKey},
        mManager{&aManager}
    {}

    template <class T_component>
    void add(T_component aComponent);

    // TODO emplace() which construct the components by forwarding arguments.

    template <class T_component>
    void remove();

    void erase();

    // TODO return an EntityRecord* (or use the nested Archetype*) allowing :
    // * to test when needed.
    // * to blindly dereference when logic requires that the record be valid.
    /// \return The EntityRecord associated with the handled entity.
    /// The record is returned by **copy**, to prevent mutation.
    EntityRecord record() const;

    EntityReference reference() const;

    Archetype & archetype() const;

    void updateRecord(EntityRecord aNewRecord);

    HandleKey<Entity> mKey;
    // Made into a pointer, so Handle<Entity>, and the types it composes, can be assigned
    // (assignment is currently a requirement on types used as components).
    EntityManager * mManager;
};


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
bool Entity::has()
{
    return mReference.mArchetype->has<T_component>();
}


template <class T_component>
T_component & Entity::get()
{
    return mReference.mArchetype->get<T_component>(mReference.mIndex);
}


} // namespace ent
} // namespace ad

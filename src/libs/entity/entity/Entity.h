#pragma once


#include "Archetype.h"
#include "HandleKey.h"
#include "entity/Component.h"

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
    Handle(HandleKey<Archetype> aKey, EntityManager & aManager) :
        mKey{aKey},
        mManager{&aManager}
    {}

    HandleKey<Archetype> mKey;
    EntityManager * mManager;
};


struct EntityRecord
{
    HandleKey<Archetype> mArchetype;
    EntityIndex mIndex; // Index of this Entity in (each store of) the Archetype.
    std::shared_ptr<std::string> mNamePtr;
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

    const char * name();

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
    template <class...> friend class Query; // Instantiates Handle on iteration.
    friend class Archetype; // So that Archetype can check handle integrity

    template <class>
    friend class Wrap;

    // TODO remove
    template <class...>
    friend class detail::QueryBackend;

public:
    /// \brief Construct a default Handle, which is not valid.
    /// \details This constructor assigns the invalid HandleKey and a
    /// static (private) EntityManager to the handle. 
    /// This EntityManager has an "invalid record" associated to the invalid HandleKey.
    /// This approach allows to not make a special case for the default constructed handle.
    Handle();

    /// \brief Checks whether the handle is valid, currently pointing to an Entity.
    // TODO not sure it should exist as a standalone, as it duplicates some logic from get()
    bool isValid() const;

    std::optional<Entity_view> get() const;

    /// \warning This handle must outlive the returned Entity.
    std::optional<Entity> get(Phase & aPhase);

    friend bool operator==(const Handle & aLhs, const Handle & aRhs)
    {
        assert (aLhs.mManager == aRhs.mManager);
        return aLhs.mKey == aRhs.mKey;
    }

    /// \attention Removes the generation, only returning the index part of the HandleKey.
    EntityIndex id() const
    {
        return mKey;
    }

    const char * name() const;

    const TypeSet getComponentsInfo() const;

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

template<>
struct std::hash<ad::ent::Handle<ad::ent::Entity>>
{
    std::size_t operator()(const ad::ent::Handle<ad::ent::Entity> aHandle) const noexcept
    {
        return std::hash<ad::ent::EntityIndex>{}(aHandle.id());
    }
};

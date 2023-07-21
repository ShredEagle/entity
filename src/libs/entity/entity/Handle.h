#pragma once

#include "HandleKey.h"
#include "Component.h"

#include <optional>

namespace ad {
namespace ent {

namespace detail {
template <class... VT_components>
class QueryBackend;
}

template <class T_handled>
class Handle;

class EntityManager;
class Archetype;
class Phase;

struct EntityRecord
{
    HandleKey<Archetype> mArchetype;
    EntityIndex mIndex; // Index of this Entity in (each store of) the Archetype.
};


struct EntityReference
{
    Archetype * mArchetype;
    EntityIndex mIndex;
};


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

class Entity;
class Entity_view;

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
}
}

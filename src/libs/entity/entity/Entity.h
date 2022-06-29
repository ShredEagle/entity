#pragma once


#include "Archetype.h"

#include <optional>

#include <cstddef>


namespace ad {
namespace ent {


class HandleKey
{
public:
    HandleKey() = default;

    operator std::size_t () const
    { return mIndex; }

    HandleKey operator++(int /*postfix*/)
    {
        return HandleKey{mIndex++};
    }

private:
    HandleKey(std::size_t aIndex) :
        mIndex{aIndex}
    {}

    std::size_t mIndex{0};
};


struct EntityRecord
{
    Archetype * mArchetype;
    EntityIndex mIndex;
};


class EntityManager; // forward

template <class T_handled>
class Handle; // forward


class Phase
{
};


class Entity
{
    friend class Handle<Entity>;

public:
    /// \warning Thread unsafe!
    // TODO this one should be made thread safe in priority (because it does not need to be deferred)
    // An idea to evaluate: could it be lock-free via read-modify-write?
    // see: https://preshing.com/20120612/an-introduction-to-lock-free-programming/
    template <class T_component>
    Entity & add(T_component aComponent);

    template <class T_component>
    bool has();

    template <class T_component>
    T_component & get();

private:
    Entity(
        EntityRecord aRecord,
        Handle<Entity> & aHandle,
        Phase & aPhase) :
        mRecord{std::move(aRecord)},
        mHandle{aHandle},
        mPhase{aPhase}
    {}

    // For immediate operations
    EntityRecord mRecord;

    // For  deferred operations
    Handle<Entity> & mHandle;
    Phase & mPhase;
};


template <class T_handled>
class Handle;


template <>
class Handle<Entity>
{
    friend class Entity;
    friend class EntityManager;

public:
    /// \important This handle must outlive the returned Entity.
    std::optional<Entity> get(Phase & aPhase);

private:
    Handle(HandleKey aKey, EntityManager & aManager) :
        mKey{aKey},
        mManager{aManager}
    {}

    template <class T_component>
    void add(T_component aComponent);

    // TODO return an EntityRecord*, allowing :
    // * to test when needed.
    // * to blindly dereference when logic requires that the record be valid.
    EntityRecord record() const;

    void updateRecord(EntityRecord aNewRecord);

    HandleKey mKey;
    EntityManager & mManager;
};



//
// Implementations
//
template <class T_component>
Entity & Entity::add(T_component aComponent)
{
    // TODO defer
    mHandle.add<T_component>(aComponent);
    return *this;
}

template <class T_component>
bool Entity::has()
{
    return mRecord.mArchetype->has<T_component>();
}


template <class T_component>
T_component & Entity::get()
{
    return mRecord.mArchetype->get<T_component>(mRecord.mIndex);
}



} // namespace ent
} // namespace ad

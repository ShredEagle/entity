#pragma once


#include "Archetype.h"

#include <functional>
#include <mutex>
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


/// \brief This class allows to stack-up the operations to be
/// deferred until the end of the phase.
///
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

    /// \brief Remove the entity itself from the EntityManager.
    void erase();

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

    void erase();

    // TODO return an EntityRecord* (or use the nested Archetype*) allowing :
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
        [&handle = mHandle, component = std::move(aComponent)]
        {
            handle.add<T_component>(component);
        });
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

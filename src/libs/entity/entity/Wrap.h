#pragma once

#include "Entity.h"


namespace ad {
namespace ent {


// Implementation note:
// Initially, we limited instantiation to T_stored that were Wrappable.
// A T_stored satisfying the Wrappable requirement could not be part of a Query var args.
/// This way, it would prevent invalidating the Wrap<> from an external query.
// Yet, for the moment this was reverted by commit #d3f5a3e (It is considered unlikely to happen).

/// @brief Wraps an instance of T_stored as a component of a dedicated (hidden) entity.
///
/// This allows to store arbitrary types' instances into the entity manager, thus providing
/// the ability to save and restore them alongside the "normal" entities.
///
/// @note Follows reference sematic. 
/// I.e. a `const Wrap` does not allow mutation of the T_stored instance.
template <class T_stored>
class Wrap
{
public:
    template <class... VT_ctorArgs>
    Wrap(EntityManager & aWorld, VT_ctorArgs &&... aCtorArgs) :
        mWrapped{aWorld.addEntity()}
    {
        // Note: one drawback is that we make one phase per initialized handle.
        // If we were doing it manually, we could share one init phase among several stores.
        Phase init;

        // If the construction expression providing the EntityManager first is valid, select it.
        if constexpr(requires {T_stored{aWorld, std::forward<VT_ctorArgs>(aCtorArgs)...};})
        {
            // TODO #emplace, emplace when entity offers it
            mWrapped.get(init)->add(T_stored{aWorld, std::forward<VT_ctorArgs>(aCtorArgs)...});
        }
        // Otherwise, call a construction expression only providing the variadic arguments.
        else
        {
            mWrapped.get(init)->add(T_stored{std::forward<VT_ctorArgs>(aCtorArgs)...});
        }
    }


    ~Wrap()
    {
        Phase destruction;
        mWrapped.get(destruction)->erase();
    }


    T_stored & operator*()
    { return get(); };

    const T_stored & operator*() const
    { return get(); };


    T_stored * operator->()
    { return &get(); }

    const T_stored * operator->() const
    { return &get(); }


private:
    T_stored & get() const
    {
        const EntityRecord & record = mWrapped.record();
        // Wrap stores a single component on the entity, so the storage index is 0.
        Storage<T_stored> & storage = mWrapped.archetype().getStorage(StorageIndex<T_stored>{0});
        // We do not check the index validity, no other code should be able to remove the entity.
        return storage[record.mIndex];
    }


    Handle<Entity> mWrapped;
};


} // namespace ent
} // namespace ad

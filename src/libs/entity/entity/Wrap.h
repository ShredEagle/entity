#pragma once

#include "Entity.h"


namespace ad {
namespace ent {


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
        // TODO emplace when entity offers it
        // TODO would be nice to automatically forward world when it is the first ctor argument
        mWrapped.get(init)->add(T_stored{/*aWorld,*/ std::forward<VT_ctorArgs>(aCtorArgs)...});
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
        // Wrap stores a single component on the entity, so the storage index is 0.
        Storage<T_stored> & storage = mWrapped.archetype().getStorage(StorageIndex<T_stored>{0});
        // We do not check the index validity, no other code should be able to remove the entity.
        return storage[mWrapped.record().mIndex];
    }


    Handle<Entity> mWrapped;
};


} // namespace ent
} // namespace ad

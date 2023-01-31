#pragma once




namespace ad {
namespace ent {


template <class T_stored>
struct Store
{
    template <class... VT_ctorArgs>
    Store(ent::EntityManager & aWorld, VT_ctorArgs &&... aCtorArgs) :
        mWrapped{aWorld.addEntity()}
    {
        // Note: one drawback is that we make one phase per initialized handle.
        // If we were doing it manually, we would share one init phase for potentially several stores.
        ent::Phase init;
        // TODO emplace when entity offers it
        // TODO would be nice to automatically forward world when it is the first ctor argument
        mWrapped.get(init)->add(T_stored{/*aWorld,*/ std::forward<VT_ctorArgs>(aCtorArgs)...});
    }


    T_stored & get(ent::Phase & aPhase) const
    {
        auto entity = mWrapped.get(aPhase);
        assert(entity);
        return entity->get<T_stored>();
    }


    T_stored & operator*() const
    {
        ent::Phase dummy;
        return get(dummy);
    };


    T_stored * operator->() const
    {
        return &(**this);
    }


    // Made mutable because there is no overload const overload of Handle<>::get()
    mutable ent::Handle<ent::Entity> mWrapped;
};


} // namespace ent
} // namespace ad

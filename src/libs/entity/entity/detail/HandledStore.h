#pragma once


#include <map>


namespace ad {
namespace ent {
namespace detail {


// Disclaimer: this is a q&d prototype of the required API,
// this will not be performant.
// TODO Implement a free list.
template <class T_data>
class HandledStore
{
public:
    using Handle = std::size_t;

    Handle insert(T_data aData);

    template <class... VT_aArgs>
    Handle emplace(VT_aArgs &&... aCtorArgs);

    void erase(Handle aHandle);

    auto begin()
    { return mStore.begin(); }
    auto end()
    { return mStore.end(); }

    auto begin() const
    { return mStore.begin(); }
    auto end() const
    { return mStore.end(); }

private:
    Handle mNextHandle{0};
    std::map<Handle, T_data> mStore;
};


//
// Implementations
//
template <class T_data>
typename HandledStore<T_data>::Handle HandledStore<T_data>::insert(T_data aData)
{
    mStore.emplace(mNextHandle, std::move(aData));
    return mNextHandle++;
}


template <class T_data>
template <class... VT_aArgs>
typename HandledStore<T_data>::Handle HandledStore<T_data>::emplace(VT_aArgs &&... aCtorArgs)
{
    mStore.emplace(mNextHandle, std::forward<VT_aArgs>(aCtorArgs)...);
    return mNextHandle++;
}


template <class T_data>
void HandledStore<T_data>::erase(Handle aHandle)
{
    mStore.erase(aHandle);
}

} // namespace detail
} // namespace ent
} // namespace ad

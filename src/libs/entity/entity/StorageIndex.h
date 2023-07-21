#pragma once

#include <cstddef>

// Note: Sadly, this does not seem to be enough to forward across function templates
//template <class T_component>
//using StorageIndex = std::size_t;

/// \brief This class allows to encapsulate the static component type
/// alongside the index of the storage for this component (in an Archetype).
///
/// This is notably useful to store "typed indices" in the tuple used by the QueryBackend.
template <class T_component>
struct StorageIndex
{
public:
    /*implicit*/ StorageIndex(std::size_t aIndex) :
        mIndex{aIndex}
    {}

    operator std::size_t & ()
    { return mIndex; }

    operator std::size_t () const
    { return mIndex; }

private:
    std::size_t mIndex;
};


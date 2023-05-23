#pragma once

#include "Component.h"
#include "HandleKey.h"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>

#include <cassert>


namespace ad {
namespace ent {


class Entity; // forward

template <class>
class Storage;

class StorageBase
{
public:
    virtual ~StorageBase() = default;

    template <class T_data>
    Storage<T_data> & as();

    template <class T_data>
    T_data & get(EntityIndex aElementId);

    virtual std::size_t size() const = 0;
    virtual void * data() = 0;
    virtual std::unique_ptr<StorageBase> cloneEmpty() const = 0;
    virtual std::unique_ptr<StorageBase> clone() const = 0;
    /// \brief Move data from aSource, pushing it a the back of this storage.
    virtual void moveFrom(EntityIndex aSourceIndex, StorageBase & aSource) = 0;

    virtual void remove(EntityIndex aSourceIndex) = 0;

    /// Intended for use with tests, to ensure the consistency of an Archetype.
    virtual ComponentId getType() = 0;


private:
    // TODO cache the pointer instead of virtual function,
    // but does not work with vectors since insertion can invalidate
    //void * mData;
};


template <class T_component>
class Storage : public StorageBase
{
public:
    std::size_t size() const override
    { return mArray.size(); }

    void * data() override
    { return mArray.data(); }

    /// @brief Non-virtual function, to access the underlying array when a fully typed Storage is available.
    T_component & operator[](std::size_t aIndex)
    { return mArray[aIndex]; }

    const T_component & operator[](std::size_t aIndex) const
    { return mArray[aIndex]; }

    std::unique_ptr<StorageBase> cloneEmpty() const override
    { return std::make_unique<Storage<T_component>>(); }

    std::unique_ptr<StorageBase> clone() const override
    { return std::make_unique<Storage<T_component>>(*this); }

    void moveFrom(EntityIndex aSourceIndex, StorageBase & aSource) override
    { mArray.push_back(std::move(aSource.get<T_component>(aSourceIndex))); }

    void remove(EntityIndex aSourceIndex) override;

    ComponentId getType() override;

    // Client should not be able to get access to Storage instances at all
//private:
    std::vector<T_component> mArray;
};


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

class EntityManager; //forward


/// \brief Actual storage for the different components of an Archetype.
///
/// It is a light wrapper around a vector of unique_ptrs, with the added
/// ability of copying (via StorageBase cloning).
class DataStore : public std::vector<std::unique_ptr<StorageBase>>
{
    using Parent_t = std::vector<std::unique_ptr<StorageBase>>;

public:
    DataStore() = default;
    ~DataStore() = default;

    DataStore(const DataStore & aRhs);
    DataStore & operator=(const DataStore & aRhs);

    DataStore(DataStore && aRhs) = default;
    DataStore & operator=(DataStore && aRhs) = default;

private:
    void swap(DataStore & aRhs);
};

// TODO Establish what it means for an Archetype to be constant:
// * At least means that entities cannot be added/removed from the archetype.
// * Additionally, could mean that the entities' components' values are constant.
// Currently, taking the second, more conservative approach (i.e., get() is not const).
// This makes Archetype a value-type.
class Archetype
{
public:
    //std::size_t getSize() const
    //{ return mSize; }
    TypeSet getTypeSet() const
    { return {mType.begin(), mType.end()}; }

    /// \brief Constructs an Archetype which extends this Archetype with component T_component
    template <class T_component>
    Archetype makeExtended() const;

    /// \brief Constructs an Archetype which restricts this Archetype, excluding component T_component
    template <class T_component>
    Archetype makeRestricted() const;

    std::size_t countEntities() const;

    bool checkStoreSize() const;

    template <class T_component>
    bool has() const;

    template <class T_component>
    T_component & get(EntityIndex aEntityIndex);

    void remove(EntityIndex aEntityIndex, EntityManager & aManager);

    // Intended for tests
    void verifyHandles(EntityManager & aManager);

    // Intended for tests
    bool verifyConsistency();

    // TODO should probably not be public
    /// \brief Move an entity from this Archetype to aDestination Archetype.
    /// \return The HandleKey of the entity which was moved to the index previously
    void move(EntityIndex aEntityIndex,
              Archetype & aDestination,
              EntityManager & aManager);

    template <class T_component>
    EntityIndex push(T_component aComponent);

    /// \attention For use by the EntityManager on the empty archetype only.
    void pushKey(HandleKey<Entity> aKey)
    { mHandles.push_back(aKey); }

    // TODO should not be public, this is an implementation detail for queries
    template <class T_component>
    StorageIndex<T_component> getStoreIndex() const;

    template <class T_component>
    Storage<T_component> & getStorage(StorageIndex<T_component> aComponentIndex);

    /// \attention implementation detail, intended for use by Query iteration.
    const std::vector<HandleKey<Entity>> getEntityIndices() const
    { return mHandles; }

private:
    //std::size_t mSize{0};
    // TODO cache typeset, or even better only have a typeset, so the components are ordered
    //TypeSet mTypeSet;
    std::vector<ComponentId> mType;
    DataStore mStores;
    // The handles of the entities stored in this archetype, in the same order than in each Store.
    std::vector<HandleKey<Entity>> mHandles;
};


//
// Implementations
//

namespace detail
{

    template <class T_element>
    void eraseByMoveOver(std::vector<T_element> & aVector, std::size_t aErasedIndex)
    {
        assert(aVector.size() > aErasedIndex);

        // Implementer note:
        // This method moves the last element of the store onto the removed index,
        // then it erases the last element.

        auto elementIt = aVector.begin() + aErasedIndex;
        // Can only remove from a non-empty storage, so end()-1 must be valid.
        auto backIt = aVector.end() - 1;

        std::move(backIt, aVector.end(), elementIt);
        aVector.pop_back();
    }


} // namespace detail


template <class T_data>
Storage<T_data> & StorageBase::as()
{
    assert(getId<T_data>() == this->getType());
    return *reinterpret_cast<Storage<T_data> *>(this);
}


template <class T_data>
T_data & StorageBase::get(EntityIndex aElementId)
{
    return as<T_data>().mArray[aElementId];
}


template <class T_component>
void Storage<T_component>::remove(EntityIndex aSourceIndex)
{
    detail::eraseByMoveOver(mArray, aSourceIndex);
}


template <class T_component>
ComponentId Storage<T_component>::getType()
{
    return getId<T_component>();
}


template <class T_component>
Archetype Archetype::makeExtended() const
{
    // TODO reuse the typeset already computed in the calling code
    // once we directly stores the typeset in the Archetype.
    Archetype result;
    result.mType = mType;
    result.mType.push_back(getId<T_component>());

    for (const auto & store : mStores)
    {
        result.mStores.push_back(store->cloneEmpty());
    }
    result.mStores.push_back(std::make_unique<Storage<T_component>>());

    return result;
}


template <class T_component>
Archetype Archetype::makeRestricted() const
{
    ComponentId retired = getId<T_component>();

    Archetype result;
    result.mType = mType;
    std::erase(result.mType, retired);

    for (const auto & store : mStores)
    {
        if(store->getType() != retired)
        {
            result.mStores.push_back(store->cloneEmpty());
        }
    }

    return result;
}


template <class T_component>
bool Archetype::has() const
{
    return std::find(mType.begin(), mType.end(), getId<T_component>()) != mType.end();
}


template <class T_component>
T_component & Archetype::get(EntityIndex aEntityIndex)
{
    assert(has<T_component>());

    for(std::size_t storeId = 0; storeId != mType.size(); ++storeId)
    {
        if(mType[storeId] == getId<T_component>())
        {
            return mStores[storeId]->get<T_component>(aEntityIndex);
        }
    }

    // TODO not sure if we should ressort to exception for this kind of situations?
    throw std::logic_error("Archetype does not have requested component.");
}


template <class T_component>
EntityIndex Archetype::push(T_component aComponent)
{
    assert(has<T_component>());

    for(std::size_t storeId = 0; storeId != mType.size(); ++storeId)
    {
        if(mType[storeId] == getId<T_component>())
        {
            auto & components = mStores[storeId]->as<T_component>().mArray;
            components.push_back(std::move(aComponent));
            return components.size() - 1;
        }
    }

    throw std::logic_error("Archetype does not have requested component.");
}


template <class T_component>
StorageIndex<T_component> Archetype::getStoreIndex() const
{
    for(std::size_t storeId = 0; storeId != mType.size(); ++storeId)
    {
        if(mType[storeId] == getId<T_component>())
        {
            return storeId;
        }
    }
    throw std::logic_error{"Archetype does not contain requested component."};
}


template <class T_component>
Storage<T_component> & Archetype::getStorage(StorageIndex<T_component> aComponentIndex)
{
    return mStores[aComponentIndex]->template as<T_component>();
}


} // namespace ent
} // namespace ad

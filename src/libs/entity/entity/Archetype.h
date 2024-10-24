#pragma once

#include "Component.h"
#include "HandleKey.h"

#if defined(ENTITY_SANITIZE)
#include <handy/AtomicVariations.h>
#endif

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
    /// \brief Copy data from aSource, pushing it a the back of this storage.
    virtual void copyFrom(EntityIndex aSourceIndex, StorageBase & aSource) = 0;

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

    // Note: pushes back into mArray, but do not remove from source.
    // Archetype::remove() will take care of that removal.
    void moveFrom(EntityIndex aSourceIndex, StorageBase & aSource) override
    { mArray.push_back(std::move(aSource.get<T_component>(aSourceIndex))); }

    void copyFrom(EntityIndex aSourceIndex, StorageBase & aSource) override
    { mArray.push_back(T_component{aSource.get<T_component>(aSourceIndex)}); }

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
    std::unique_ptr<Archetype> makeExtended() const;

    /// \brief Constructs an Archetype which restricts this Archetype, excluding component T_component
    template <class T_component>
    std::unique_ptr<Archetype> makeRestricted() const;

    std::size_t countEntities() const;

    template <class T_component>
    bool has() const;

    template <class T_component>
    T_component & get(EntityIndex aEntityIndex);

    void remove(EntityIndex aEntityIndex, EntityManager & aManager);

    /// \brief Intended for tests, makes sure that each handle in this Archetype
    /// corresponds to an Entity whose archetype is this instance.
    bool verifyHandlesConsistency(EntityManager & aManager);

    // Intended for tests
    bool verifyStoresConsistency();

    // TODO should probably not be public
    /// \brief Move an entity from `this` Archetype (i.e. the source) to `aDestination` Archetype.
    /// Will only attempt to move the common components.
    /// \param aEntityIndex is the index of the moved entity in the source, i.e. in `this` Archetype.
    /// \return The HandleKey of the entity which was moved to the index previously
    /// \warning If source and destination are the same Archetype (`aDestination` Archetype is `this` archetype),
    /// the archetype will be left unchanged by the operation.
    void move(EntityIndex aEntityIndex,
              Archetype & aDestination,
              EntityManager & aManager);

    void copy(EntityIndex aSourceEntityIndex, HandleKey<Entity> aDestHandle, Archetype & aDestination, EntityManager & aManager);

    template <class T_component>
    EntityIndex push(T_component aComponent);

    /// \attention For use by the EntityManager on the empty archetype only.
    void pushKey(HandleKey<Entity> aKey);

    // TODO should not be public, this is an implementation detail for queries
    template <class T_component>
    StorageIndex<T_component> getStoreIndex() const;

    template <class T_component>
    Storage<T_component> & getStorage(StorageIndex<T_component> aComponentIndex);

    /// \attention implementation detail, intended for use by Query iteration.
    const std::vector<HandleKey<Entity>> & getEntityIndices() const
    { return mHandles; }

#if defined(ENTITY_SANITIZE)
    CopyableAtomic<unsigned int> mCurrentQueryIterations{0};
#endif

private:
    enum class Operation
    {
      Move,
      Copy,
    };

    /// \brief base template for archetype copy and move
    template<Operation N_operation>
    void moveOrCopy(EntityIndex aSourceEntityIndex, Archetype & aDestination, EntityManager & aManager);

    std::unique_ptr<Archetype> makeRestrictedFromTypeId(const ComponentId aId) const;


    /// \brief Intended for tests, makes sure that each store size matche the count of handles.
    bool checkStoreSize() const;

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
std::unique_ptr<Archetype> Archetype::makeExtended() const
{
    // TODO reuse the typeset already computed in the calling code
    // once we directly stores the typeset in the Archetype.
    auto result = std::make_unique<Archetype>();
    result->mType = mType;
    result->mType.push_back(getId<T_component>());

    for (const auto & store : mStores)
    {
        result->mStores.push_back(store->cloneEmpty());
    }
    result->mStores.push_back(std::make_unique<Storage<T_component>>());

    return result;
}


template <class T_component>
std::unique_ptr<Archetype> Archetype::makeRestricted() const
{
    ComponentId retired = getId<T_component>();
    return makeRestrictedFromTypeId(retired);
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

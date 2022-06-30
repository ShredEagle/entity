#pragma once

#include "Component.h"

#include <memory>
#include <stdexcept>
#include <vector>

#include <cassert>


namespace ad {
namespace ent {


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

    std::unique_ptr<StorageBase> cloneEmpty() const override
    { return std::make_unique<Storage<T_component>>(); }

    void moveFrom(EntityIndex aSourceIndex, StorageBase & aSource) override
    { mArray.push_back(std::move(aSource.get<T_component>(aSourceIndex))); }

    void remove(EntityIndex aSourceIndex) override;

    ComponentId getType() override;

    // Client should not be able to get access to Storage instances at all
//private:
    std::vector<T_component> mArray;
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
    Archetype makeAdded() const;

    std::size_t countEntities() const;

    template <class T_component>
    bool has() const;

    template <class T_component>
    T_component & get(EntityIndex aEntityIndex);

    void remove(EntityIndex aEntityIndex);

    // Intended for tests
    bool verifyConsistency();

    // TODO should probably not be public
    /// \brief Move an entity from this Archetype to aDestination Archetype.
    void move(EntityIndex aEntityIndex, Archetype & aDestination);

    template <class T_component>
    EntityIndex push(T_component aComponent);

private:

    //std::size_t mSize{0};
    // TODO cache typeset, or even better only have a typeset, so the components are ordered
    //TypeSet mTypeSet;
    std::vector<ComponentId> mType;
    std::vector<std::unique_ptr<StorageBase>> mStores;
};


//
// Implementations
//
template <class T_data>
Storage<T_data> & StorageBase::as()
{
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
    // Implementer note:
    // This method moves the last element of the store onto the removed index,
    // then it erases the last element.

    assert(size() > aSourceIndex);

    auto elementIt = mArray.begin() + aSourceIndex;
    // Can only remove from a non-empty storage, so end()-1 must be valid.
    auto backIt = mArray.end() - 1;

    std::move(backIt, mArray.end(), elementIt);
    mArray.erase(backIt);
}


template <class T_component>
ComponentId Storage<T_component>::getType()
{
    return getId<T_component>();
}


template <class T_component>
Archetype Archetype::makeAdded() const
{
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


} // namespace ent
} // namespace ad

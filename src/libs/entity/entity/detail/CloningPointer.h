#pragma once


#include <memory>


namespace ad {
namespace ent {
namespace detail {


/// \brief A unique pointer with value semantic.
/// Each instance uniquely owns a different copy of the value after copy operations.
///
/// \attention Naive implementation, not intended for polymorphic values
/// (see: https://stackoverflow.com/a/55152112/1027706)
template <class T>
class CloningPointer : public std::unique_ptr<T>
{
    using Parent_t = std::unique_ptr<T>;

public:
    //using Parent_t::Parent_t;

    CloningPointer(Parent_t aParent) :
        Parent_t{std::move(aParent)}
    {}

    CloningPointer(const CloningPointer & aRhs) :
        CloningPointer{std::make_unique<T>(*aRhs)}
    {}

    CloningPointer & operator=(const CloningPointer & aRhs)
    {
        CloningPointer copy{aRhs};
        swap(copy);
        return *this;
    }

    CloningPointer(CloningPointer && aRhs) = default;
    CloningPointer & operator=(CloningPointer && aRhs) = default;

private:
    void swap(CloningPointer & aRhs)
    {
        std::swap(static_cast<Parent_t &>(*this),
                  static_cast<Parent_t &>(aRhs));
    }
};


} // namespace detail
} // namespace ent
} // namespace ad

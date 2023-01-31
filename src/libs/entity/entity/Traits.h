#pragma once


namespace ad {
namespace ent {


template <class>
struct is_wrappable : public std::false_type
{};


template <class T>
    requires T::is_wrappable
struct is_wrappable<T> : public std::true_type
{};


template <class T>
concept Wrappable = requires {is_wrappable<T>::value;};


} // namespace ent
} // namespace ad

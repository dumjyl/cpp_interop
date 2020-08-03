/// \file
/// Core utilities for ensnare.

#pragma once

#include "sugar.hpp"

#include <memory>
#include <unordered_map>

using namespace sugar;

namespace ensnare {
/// Ensnare's goto hash table
template <typename K, typename V> using Map = std::unordered_map<K, V>;

/// Ensnare's goto ref counting pointer.
template <typename T> using Node = std::shared_ptr<T>;

/// Construct a ref counted node with `Y(args...)`.
template <typename Y, typename... Args> Node<Y> node(Args... args) {
   return std::make_shared<Y>(args...);
}

/// Ensnare's goto class for encoding non-existence for refernces.
template <typename T> using OptRef = Opt<std::reference_wrapper<T>>;

/// Are the pointer addresses of two references equal?
template <typename T> bool pointer_equality(T& a, T& b) {
   return reinterpret_cast<std::uintptr_t>(std::addressof(a)) ==
          reinterpret_cast<std::uintptr_t>(std::addressof(b));
}

/// Check if a `self` is of variant/type `T`. If it is, it safely be read like: `deref<T>(self)`.
template <typename T, typename... Variants> bool is(const Node<Union<Variants...>>& self) {
   return sugar::is<T, Variants...>(*self);
}

/// An immutable reference to the variant `T`. This will throw if the wrong variant is accessed.
template <typename T, typename... Variants> const T& as(const Node<Union<Variants...>>& self) {
   return sugar::as<T, Variants...>(*self);
}
}; // namespace ensnare

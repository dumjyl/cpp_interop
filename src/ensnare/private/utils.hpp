#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
// Template recursion base case.
template <typename... Args> fn write(std::ostream* out, Args... args) {}

// Write some arguments to a output stream.
template <typename Arg, typename... Args> fn write(std::ostream* out, Arg arg, Args... args) {
   *out << arg;
   write(out, args...);
}

/// Write some `args` to the ouput stream `out` with the `<<` operator.
template <typename... Args> fn print(std::ostream* out, Args... args) {
   write(out, args...);
   write(out, '\n');
}

/// Write some `args` to the ouput stream `std::cout` with the `<<` operator.
template <typename... Args> fn print(Args... args) { print(&std::cout, args...); }

/// Print `args` and exit the application without the possibility of recovery.
template <typename... Args> fn fatal [[noreturn]] (Args... args) {
   print("fatal-error: ", args...);
   std::exit(1);
}

/// Ensnare's goto growable heap buffer.
template <typename T> using Vec = std::vector<T>;

/// Ensnare's goto hash table.
template <typename K, typename V> using Map = std::unordered_map<K, V>;

/// Ensnare's goto byte character buffer.
using Str = std::string;

/// Ensnare's goto ref counting pointer.
template <typename T> using Node = std::shared_ptr<T>;

/// Construct a ref counted node with `Y(args...)`.
template <typename Y, typename... Args> fn node(Args... args) -> Node<Y> {
   return std::make_shared<Y>(args...);
}

/// Ensnare's prefered way method of polymorphic object.
template <typename... Variants> using Union = const std::variant<Variants...>;

/// Check if a `self` is of variant/type `T`. If it is, it safely be read like: `deref<T>(self)`.
template <typename T, typename... Variants> fn is(const Node<Union<Variants...>>& self) -> bool {
   return std::holds_alternative<T, Variants...>(*self);
}

/// An immutable reference to the variant `T`. This will throw if the wrong variant is accessed.
template <typename T, typename... Variants>
fn deref(const Node<Union<Variants...>>& self) -> const T& {
   return std::get<T, Variants...>(*self);
}

/// Ensnare's goto class for encoding non-existence.
template <typename T> using Opt = std::optional<T>;

/// Extract the value from `self` or exit fatally with `msg`.
template <typename T> fn expect(Opt<T> self, const Str& msg) -> T {
   if (self) {
      return *self;
   } else {
      fatal("failed to unpack a none variant; " + msg);
   }
}
}; // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

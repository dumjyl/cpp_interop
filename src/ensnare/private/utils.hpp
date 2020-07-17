#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// preserve order
#include "ensnare/private/syn.hpp"

namespace ensnare {
template <typename T> using Vec = std::vector<T>;

template <typename T> using Opt = std::optional<T>;

template <typename K, typename V> using Map = std::unordered_map<K, V>;

using Str = std::string;

template <typename T> using Node = std::shared_ptr<T>;

template <typename... Variants> using Union = const std::variant<Variants...>;

// Check if a `Union` is a thing. If it is you can `deref<T>(self)` it.
template <typename T, typename... Variants> fn is(const Node<Union<Variants...>>& self) -> bool {
   return std::holds_alternative<T, Variants...>(*self);
}

template <typename T, typename... Variants>
fn deref(const Node<Union<Variants...>>& self) -> const T& {
   return std::get<T, Variants...>(*self);
}

// Create a value ref counted node of type `Y` constructed from `X`.
template <typename Y, typename... Args> fn node(Args... args) -> Node<Y> {
   return std::make_shared<Y>(args...);
}

// Template recursion base case.
template <typename... Args> fn write(std::ostream* out, Args... args) {}

// Write some arguments to a output stream.
template <typename Arg, typename... Args> fn write(std::ostream* out, Arg arg, Args... args) {
   *out << arg;
   write(out, args...);
}

// Write some arguments to an ouput stream with a newline.
template <typename... Args> fn print(std::ostream* out, Args... args) {
   write(out, args...);
   write(out, '\n');
}

// Write some arguments to an `std::cout` with a newline.
template <typename... Args> fn print(Args... args) { print(&std::cout, args...); }

// Write some arguments to `std::cout` exit with error code 1.
template <typename... Args> fn fatal [[noreturn]] (Args... args) {
   print("fatal-error: ", args...);
   std::exit(1);
}
}; // namespace ensnare
#include "ensnare/private/undef_syn.hpp"

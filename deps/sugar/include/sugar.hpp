#pragma once

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace sugar {
// good stuff: https://florianjw.de/en/variadic_templates.html
template <typename... Args> void discard(Args... args) {}

// and again: https://florianjw.de/en/passing_overloaded_functions.html
#define LAMBDA(...)                                                                                \
   [](auto&&... args) -> decltype(auto) {                                                          \
      return __VA_ARGS__(std::forward<decltype(args)>(args)...);                                   \
   }

/// Write some arguments to a output stream.
template <typename... Args> void write(std::ostream& out, Args... args) {
   discard(((void)(out << args), 0)...);
}

/// Write some arguments to `std::cout` stream.
template <typename... Args> void write(Args... args) { write(std::cout, args...); }

/// Write some `args` to the ouput stream `out` with the `<<` operator.
template <typename... Args> void print(std::ostream& out, Args... args) {
   write(out, args...);
   out << '\n';
}

/// Write some `args` to the ouput stream `std::cout` with the `<<` operator.
template <typename... Args> void print(Args... args) { print(std::cout, args...); }

/// Write some `args` to the ouput stream `std::cerr` with the `<<` operator.
template <typename... Args> void print_err(Args... args) { print(std::cerr, args...); }

/// Print `args` and exit the application without the possibility of recovery.
template <typename... Args> void fatal [[noreturn]] (Args... args) {
   print("fatal error: ", args...);
   std::exit(1);
}

template <typename... Args> inline void require(bool cond, Args... args) {
   if (!cond) {
      fatal(args...);
   }
}

using U8 = std::uint8_t;
using U16 = std::uint16_t;
using U32 = std::uint32_t;
using U64 = std::uint64_t;
using I8 = std::int8_t;
using I16 = std::int16_t;
using I32 = std::int32_t;
using I64 = std::int64_t;
using F32 = float;
using F64 = double;
using Size = std::size_t;

static_assert(sizeof(F32) == 4);
static_assert(sizeof(F64) == 8);

template <typename... Variants> using Union = std::variant<Variants...>;

using Str = std::string;
template <typename T> using Vec = std::vector<T>;
template <typename... Ts> using Tuple = std::tuple<Ts...>;
template <typename T> using Opt = std::optional<T>;

/// Extract the value from `self` or exit fatally with `msg`.
template <typename T, typename... Msgs> T expect(Opt<T> self, Msgs... msgs) {
   require(bool(self), "failed to unpack a none variant; ", msgs...);
}

/// Check if a `self` is of variant/type `T`. If it is, it safely be read like: `as<T>(self)`.
template <typename T, typename... Variants> bool is(const Union<Variants...>& self) {
   return std::holds_alternative<T, Variants...>(self);
}

/// An immutable reference to the variant `T`. This will throw if the wrong variant is accessed.
template <typename T, typename... Variants> const T& as(const Union<Variants...>& self) {
   return std::get<T, Variants...>(self);
}

namespace detail {
template <typename T> class BeginEnd {
   private:
   T _begin;
   T _end;

   public:
   BeginEnd(T begin, T end) : _begin(begin), _end(end) {}
   T begin() { return _begin; }
   T end() { return _end; }
};

template <typename T> class OrdinalIter {
   private:
   T cur;

   public:
   OrdinalIter(T cur) : cur(cur) {}
   bool operator!=(OrdinalIter<T> other) { return cur < other.cur; }
   T operator*() { return cur; }
   void operator++() { ++cur; }
};
} // namespace detail

/// An iterator that yields ordinal values in the range [first, last].
template <typename T> detail::BeginEnd<detail::OrdinalIter<T>> range(T first, T last) {
   return detail::BeginEnd(detail::OrdinalIter(first), detail::OrdinalIter(last + 1));
}

/// An iterator that yields ordinal values in the range [0, last].
template <typename T> detail::BeginEnd<detail::OrdinalIter<T>> range(T last) {
   return range<T>(0, last);
}
} // namespace sugar

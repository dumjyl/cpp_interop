#pragma once

#include "ensnare/private/utils.hpp"

#include <limits>
#include <type_traits>

// preserve order
#include "ensnare/private/syn.hpp"

namespace ensnare {
namespace detail {
template <typename T, typename std::enable_if_t<std::is_signed_v<T>, int> = 0>
constexpr fn signed_bias() -> std::size_t {
   // FIXME: this might be wrong / unoptimized for some enums.
   return static_cast<std::size_t>(1) << (sizeof(T) * 8 - 1);
}

template <typename T, typename std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
constexpr fn signed_bias() -> std::size_t {
   return 0;
}

template <typename T> constexpr fn encode(T val) -> std::size_t {
   return static_cast<std::size_t>(val) ^ signed_bias<T>();
}

template <typename T> constexpr fn decode(std::size_t val) -> T {
   return static_cast<T>(val) ^ static_cast<T>(signed_bias<T>());
}

template <typename T, std::enable_if_t<std::numeric_limits<T>::is_integer, int> = 0>
constexpr fn type_size() -> std::size_t {
   return encode(std::numeric_limits<T>::max()) - encode(std::numeric_limits<T>::min()) + 1;
}

template <typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr fn type_size() -> std::size_t {
   return type_size<std::underlying_type_t<T>>();
}

// Overload constraint: Is `T` reasonably encodable in a BitSet.
template <typename T> using IsBitEncodable = std::enable_if_t<(type_size<T>() < 1024)>;
} // namespace detail

template <typename T, typename = detail::IsBitEncodable<T>> class BitSet {
   priv char detail[detail::type_size<T>() / 8];

   pub fn incl(T val) {
      auto i = detail::encode(val);
      detail[i / 8] = detail[i / 8] | (1 << (i % 8));
   }

   fn get(std::size_t i) const -> bool { return (detail[i / 8] & (1 << (i % 8))) != 0; }

   pub fn operator[](T val) const -> bool { return get(detail::encode(val)); }

   pub fn to_str() -> Str {
      Str result = "[";
      for (auto i = 0; i < detail::type_size<T>(); i += 1) {
         if (get(i)) {
            result += "1";
         } else {
            result += "0";
         }
      }
      result += "]";
   }
};

using CharSet = BitSet<char>;
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

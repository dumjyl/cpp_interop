#pragma once

#include "ensnare/private/utils.hpp"

#include <limits>
#include <type_traits>

namespace ensnare {
namespace detail {
template <typename T, typename std::enable_if_t<std::is_signed_v<T>, int> = 0>
constexpr Size signed_bias() {
   // FIXME: this might be wrong / unoptimized for some enums.
   return static_cast<Size>(1) << (sizeof(T) * 8 - 1);
}

template <typename T, typename std::enable_if_t<std::is_unsigned_v<T>, int> = 0>
constexpr Size signed_bias() {
   return 0;
}

template <typename T> constexpr Size encode(T val) {
   return static_cast<Size>(val) ^ signed_bias<T>();
}

template <typename T> constexpr T decode(std::size_t val) {
   return static_cast<T>(val) ^ static_cast<T>(signed_bias<T>());
}

template <typename T, typename std::enable_if_t<std::numeric_limits<T>::is_integer, int> = 0>
constexpr Size type_size() {
   return encode(std::numeric_limits<T>::max()) - encode(std::numeric_limits<T>::min()) + 1;
}

template <typename T, typename std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr Size type_size() {
   return type_size<std::underlying_type_t<T>>();
}

// Overload constraint: Is `T` reasonably encodable in a BitSet.
template <typename T> using IsBitEncodable = std::enable_if_t<(type_size<T>() < 1024)>;
} // namespace detail

/// A simple bit set class.
template <typename T, typename = detail::IsBitEncodable<T>> class BitSet {
   private:
   static constexpr Size array_size() { return detail::type_size<T>() / 8; }
   char detail[array_size()];

   bool get(Size i) const { return (detail[i / 8] & (1 << (i % 8))) != 0; }

   public:
   BitSet() {
      for (auto i = 0; i < array_size(); i += 1) {
         detail[i] = char(0);
      }
   }

   /// Include a value in the bitset.
   void incl(T val) {
      auto i = detail::encode(val);
      detail[i / 8] |= (1 << (i % 8));
   }

   /// Check if val is occupied.
   bool operator[](T val) const { return get(detail::encode(val)); }

   /// Stringify the bitset for debugging.
   Str to_string() {
      Str result = "[";
      for (auto i = 0; i < detail::type_size<T>(); i += 1) {
         if (get(i)) {
            result += "1";
         } else {
            result += "0";
         }
      }
      result += "]";
      return result;
   }
};

/// A bit set specialized for encoding `char`.
using CharSet = BitSet<char>;
} // namespace ensnare

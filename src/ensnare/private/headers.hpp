/// \file
/// Utilities for looking up and managing header files.
///

#pragma once

#include "ensnare/private/utils.hpp"

namespace ensnare {
/// Represents an unresolved header. Once a header is resolved it is just a file.
class Header {
   private:
   bool is_system; ///< Was this header declared as a system header with <>
   Str name;

   public:
   operator Str() const;

   /// Construct a system header.
   static Header system(const Str name);

   /// Construct a non-system header.
   Header(const Str name);

   /// Try to parse a header.
   static Opt<Header> parse(const Str& str);

   /// Query system search paths using a c++ compiler and some hacky heuristics.
   static Vec<Str> search_paths();

   /// Render this header as it would appear in a c++ header.
   Str render() const;
};
} // namespace ensnare

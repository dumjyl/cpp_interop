/// \file
/// Utilities for looking up and managing header files.
///

#pragma once

#include "ensnare/private/utils.hpp"

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
/// Represents an unresolved header. Once a header is resolved it is just a file.
class Header {
   priv bool is_system; ///< Was this header declared as a system header with <>
   priv Str name;

   pub operator Str() const;

   /// Construct a system header.
   pub static fn system(const Str name) -> Header;

   /// Construct a non-system header.
   pub Header(const Str name);

   /// Try to parse a header.
   pub static fn parse(const Str& str) -> Opt<Header>;

   /// Query system search paths using a c++ compiler and some hacky heuristics.
   pub static fn search_paths() -> Vec<Str>;

   /// Render this header as it would appear in a c++ header.
   pub fn render() const -> Str;
};
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

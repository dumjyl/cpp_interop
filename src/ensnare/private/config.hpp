/// \file
/// Handles any configurable elements of ensnare.

#pragma once

#include "ensnare/private/headers.hpp"
#include "ensnare/private/os_utils.hpp"
#include "ensnare/private/utils.hpp"

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
/// A configuration class responsible for managing command line options.
class Config {
   priv Vec<Header> _headers;
   priv Str _output;
   priv Vec<Str> _user_clang_args;
   priv Vec<Str> _include_dirs;
   priv Vec<Str> _syms;
   priv Vec<Str> _gensym_types;
   priv bool _disable_includes;
   priv bool _fold_type_suffix;
   priv bool _ignore_const;

   /// The output location.
   /// It is the first positional argument.
   pub fn output() const -> const Str&;
   /// The headers requested for binding generation.
   /// Controlled by passing header looking things (abc.h, <string>) as positional arguments.
   pub fn headers() const -> const Vec<Header>&;
   /// The user's arguments that will be passed to clang.
   /// These are the postional arguments that are not the output location or a header.
   pub fn user_clang_args() const -> const Vec<Str>&;
   pub fn include_dirs() const -> const Vec<Str>&;
   /// Specifies specific symbols to bind instead of trying to be smart.
   pub fn syms() const -> const Vec<Str>&;
   pub fn gensym_types() const -> const Vec<Str>&;
   /// If we should try to find some reasonable include search paths from a compiler.
   pub fn disable_includes() const -> bool;
   pub fn fold_type_suffix() const -> bool;
   pub fn ignore_const() const -> bool;
   /// Make a Config from unparsed command line parameters.
   pub Config(int argc, const char* argv[]);
   /// A header file with all the headers"()" rendered together.
   pub fn header_file() const -> Str;
   /// Dump the config to stdout.
   pub fn dump() const -> void;
};
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

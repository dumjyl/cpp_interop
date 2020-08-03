/// \file
/// Handles any configurable elements of ensnare.

#pragma once

#include "ensnare/private/headers.hpp"
#include "ensnare/private/utils.hpp"
#include "sugar/os_utils.hpp"

namespace ensnare {
/// A configuration class responsible for managing command line options.
class Config {
   private:
   Vec<Header> _headers;
   Path _output;
   Vec<Str> _user_clang_args;
   Vec<Str> _include_dirs;
   Vec<Str> _syms;
   Vec<Str> _gensym_types;
   bool _disable_includes;
   bool _fold_type_suffix;
   bool _ignore_const;

   public:
   /// The output location.
   /// It is the first positional argument.
   const Path& output() const;
   /// The headers requested for binding generation.
   /// Controlled by passing header looking things (abc.h, <string>) as positional arguments.
   const Vec<Header>& headers() const;
   /// The user's arguments that will be passed to clang.
   /// These are the postional arguments that are not the output location or a header.
   const Vec<Str>& user_clang_args() const;
   const Vec<Str>& include_dirs() const;
   /// Specifies specific symbols to bind instead of trying to be smart.
   const Vec<Str>& syms() const;
   const Vec<Str>& gensym_types() const;
   /// If we should try to find some reasonable include search paths from a compiler.
   bool disable_includes() const;
   bool fold_type_suffix() const;
   bool ignore_const() const;
   /// Make a Config from unparsed command line parameters.
   Config(int argc, const char* argv[]);
   /// A header file with all the headers"()" rendered together.
   Str header_file() const;
   /// Dump the config to stdout.
   void dump() const;
};
} // namespace ensnare

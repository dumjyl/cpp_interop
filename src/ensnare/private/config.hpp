#pragma once

#include "ensnare/private/os_utils.hpp"
#include "ensnare/private/utils.hpp"

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
class Config {
   priv Vec<Str> _headers;
   pub fn headers() const -> const Vec<Str>&;
   priv Str _output;
   pub fn output() const -> const Str&;
   priv Vec<Str> _clang_args;
   pub fn clang_args() const -> const Vec<Str>&;
   priv Vec<Str> _syms;
   pub fn syms() const -> const Vec<Str>&;
   priv bool _disable_includes;
   pub fn disable_includes() const -> bool;
   pub Config(int argc, const char* argv[]);
   // A header file with all the headers added together.
   pub fn header_file() const -> Str;
};
} // namespace ensnare
#include "ensnare/private/undef_syn.hpp"

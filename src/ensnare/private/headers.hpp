#pragma once

#include "ensnare/private/utils.hpp"

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
class Header {
   priv bool is_system;
   priv Str name;

   pub static fn system(const Str name) -> Header;

   pub Header(const Str name);
};

// Parse include paths out of some verbose compiler logs.
fn include_paths() -> Vec<Str>;
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

#pragma once

#include "ensnare/private/utils.hpp"

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
// split by '\n' for line processing.
fn split_newlines(const Str& str) -> Vec<Str>;
// Check if `self` ends with `suffix`.
fn ends_with(const Str& self, const Str& suffix) -> bool;
fn replace(const Str& str, const Str& find, const Str& replace) -> Str;
fn is_ident_chars(const Str& str) -> bool;
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

#pragma once

#include "ensnare/private/utils.hpp"

namespace ensnare {
// split by '\n' for line processing.
Vec<Str> split_newlines(const Str& str);
// Check if `self` ends with `suffix`.
bool ends_with(const Str& self, const Str& suffix);
Str replace(const Str& str, const Str& find, const Str& replace);
bool is_ident_chars(const Str& str);
} // namespace ensnare

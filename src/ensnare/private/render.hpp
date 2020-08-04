#pragma once

#include "ensnare/private/decl.hpp"
#include "ensnare/private/utils.hpp"

namespace ensnare {
Str render(const Vec<TypeDecl>&);
Str render(const Vec<RoutineDecl>&);
Str render(const Vec<VariableDecl>&);
} // namespace ensnare

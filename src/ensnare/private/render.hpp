#pragma once

#include "ensnare/private/ir.hpp"
#include "ensnare/private/utils.hpp"

namespace ensnare {
Str render(const Vec<Node<TypeDecl>>&);
Str render(const Vec<Node<RoutineDecl>>&);
Str render(const Vec<Node<VariableDecl>>&);
} // namespace ensnare

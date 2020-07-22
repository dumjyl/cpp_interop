#pragma once

#include "ensnare/private/ir.hpp"
#include "ensnare/private/utils.hpp"

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
fn render(const Vec<Node<TypeDecl>>&) -> Str;
fn render(const Vec<Node<RoutineDecl>>&) -> Str;
fn render(const Vec<Node<VariableDecl>>&) -> Str;
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

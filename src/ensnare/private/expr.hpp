#pragma once

#include "ensnare/private/sym.hpp"
#include "ensnare/private/utils.hpp"

namespace ensnare {
template <typename T> class LitExpr;
class ConstParamExpr;

/// Used within an Expr.
using ExprObj = Union<LitExpr<U64>, LitExpr<I64>, ConstParamExpr>;

/// Roughly maps to clang::Expr. This is currently only used to manage constant generic expressions
/// that we cannot resolve. It may be used to translate templates or even entire headers wholesale.
using Expr = Node<ExprObj>;

/// An expression that represents a literal of type `T`.
template <typename T> class LitExpr {
   public:
   const T value;
   LitExpr(T value) : value(value) {}
};

/// This is a Sym that refers to a constant template/generic parameter.
class ConstParamExpr {
   public:
   const Sym name;
   ConstParamExpr(Sym name);
};

template <typename T> Expr new_Expr(T expr) { return node<ExprObj>(expr); }
} // namespace ensnare

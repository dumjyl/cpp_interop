#pragma once

#include "ensnare/private/sym.hpp"
#include "ensnare/private/expr.hpp"
#include "ensnare/private/utils.hpp"

namespace ensnare {
class PtrType;
class RefType;
class OpaqueType;
class InstType;
class UnsizedArrayType;
class ArrayType;
class FuncType;
class ConstType;

/// Some kind of type. Should be stored within a Node.
using TypeObj = Union<Sym, PtrType, RefType, OpaqueType, InstType, UnsizedArrayType, ArrayType,
                      FuncType, ConstType>;

using Type = Node<TypeObj>;

template <typename T> Type new_Type(T type) { return node<TypeObj>(type); }

/// Just a raw pointer: `T*`
class PtrType {
   public:
   const Type pointee;
   PtrType(Type pointee);
};

/// A c++ reference pointer: `T&`
class RefType {
   public:
   const Type pointee;
   RefType(Type pointee);
};

/// Kinda dumb. What is it?
class OpaqueType {};

/// A generic instantiation: `T<A, B, C>`
class InstType {
   public:
   const Sym name;
   const Vec<Type> types;
   InstType(Sym name, Vec<Type> types);
};

/// An unsized array that should appear as a parameter or the last member of a struct: `T foo[]`
class UnsizedArrayType {
   public:
   const Type type;
   UnsizedArrayType(Type type);
};

/// A c array with a size. It may be a literal size or depend on a template paramter:
/// `T foo[123]` or `T foo[size]`
class ArrayType {
   public:
   const Expr size;
   const Type type;
   ArrayType(Expr size, Type type);
};

///
class FuncType {
   public:
   const Vec<Type> params;
   const Type return_type;
   FuncType(Vec<Type> params, Type return_type);
};

class ConstType {
   public:
   const Type type;
   ConstType(Type type);
};
} // namespace ensnare

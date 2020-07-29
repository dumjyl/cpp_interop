/// \file
/// All of the types that are not Unions appear inside a `Node<_>` or a `Node<Union<_>>`
///
/// This is a symbolized IR. Prudence must be taken to not produce distinct nodes for the same
/// entity.

#pragma once

#include "ensnare/private/utils.hpp"

#include <cstdint>

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
/// This is reference counted string class with a history. Should be stored within a node.
class Sym {
   priv Vec<Str> detail;
   priv bool _no_stropping; ///< This symbol does not require stropping even if it is a keyword.
   pub Sym(Str name, bool no_stropping = false);
   pub void update(Str name);
   pub fn latest() const -> Str;
   pub fn no_stropping() const -> bool;
};

class PtrType;
class RefType;
class OpaqueType;
class InstType;
class UnsizedArrayType;
class ArrayType;
class FuncType;

/// Some kind of type. Should be stored within a Node.
using Type =
    Union<Node<Sym>, PtrType, RefType, OpaqueType, InstType, UnsizedArrayType, ArrayType, FuncType>;

/// Just a raw pointer.
class PtrType {
   pub const Node<Type> pointee;
   pub PtrType(Node<Type> pointee);
};

/// A c++ reference pointer.
class RefType {
   pub const Node<Type> pointee;
   pub RefType(Node<Type> pointee);
};

class OpaqueType {};

class InstType {
   pub const Node<Sym> name;
   pub const Vec<Node<Type>> types;
   pub InstType(Node<Sym> name, Vec<Node<Type>> types);
};

class UnsizedArrayType {
   pub const Node<Type> type;
   pub UnsizedArrayType(Node<Type> type);
};

class ArrayType {
   pub const std::uint64_t size;
   pub const Node<Type> type;
   pub ArrayType(std::uint64_t size, Node<Type> type);
};

class FuncType {
   pub const Vec<Node<Type>> formals;
   pub const Node<Type> return_type;
   pub FuncType(Vec<Node<Type>> formals, Node<Type> return_type);
};

/// A `type Foo = Bar[X, Y]` like declaration.
/// These come from c++ declarations like `using Foo = Bar[X, Y];` or `typedef Bar[X, Y] Foo;`
class AliasTypeDecl {
   pub const Node<Sym> name;
   pub const Node<Type> type;
   pub AliasTypeDecl(Str name, Node<Type> type);
   pub AliasTypeDecl(Node<Sym> name, Node<Type> type);
};

/// An enum field declation.
class EnumFieldDecl {
   pub const Node<Sym> name;
   pub const Opt<std::int64_t> val;
   pub EnumFieldDecl(Str name);
   pub EnumFieldDecl(Str name, std::int64_t val);
};

/// A c++ enum of some kind. It may not be mapped as a nim enum.
class EnumTypeDecl {
   pub const Node<Sym> name;
   pub const Str cpp_name;
   pub const Str header;
   pub const Vec<EnumFieldDecl> fields;
   pub EnumTypeDecl(Str name, Str cpp_name, Str header, Vec<EnumFieldDecl> fields);
   pub EnumTypeDecl(Node<Sym> name, Str cpp_name, Str header, Vec<EnumFieldDecl> fields);
};

class RecordFieldDecl {
   pub const Node<Sym> name;
   pub const Node<Type> type;
   pub RecordFieldDecl(Str name, Node<Type> type);
};

/// A `struct` / `class` / `union` type declaration.
class RecordTypeDecl {
   pub const Node<Sym> name;
   pub const Str cpp_name;
   pub const Str header;
   pub const Vec<RecordFieldDecl> fields;
   pub RecordTypeDecl(Str name, Str cpp_name, Str header, Vec<RecordFieldDecl> fields);
   pub RecordTypeDecl(Node<Sym> name, Str cpp_name, Str header, Vec<RecordFieldDecl> fields);
};

/// Some kind of type declaration. Should be stored within a Node.
using TypeDecl = Union<AliasTypeDecl, EnumTypeDecl, RecordTypeDecl>;

fn name(Node<TypeDecl> decl) -> Sym&;

/// A routine parameter declaration.
class ParamDecl {
   priv Node<Sym> _name;
   priv Node<Type> _type;
   pub fn name() const -> Node<Sym>;
   pub fn type() const -> Node<Type>;
   pub ParamDecl(Str name, Node<Type> type);
   pub ParamDecl(Node<Sym> name, Node<Type> type);
};

/// A non method function declaration.
class FunctionDecl {
   pub const Node<Sym> name;
   pub const Str cpp_name;
   pub const Str header;
   pub const Vec<ParamDecl> formals;
   pub const Opt<Node<Type>> return_type;
   pub FunctionDecl(Str name, Str cpp_name, Str header, Vec<ParamDecl> formals,
                    Opt<Node<Type>> return_type);
};

/// A c++ class/struct/union constructor.
class ConstructorDecl {
   pub const Str cpp_name;
   pub const Str header;
   pub const Node<Type> self;
   pub const Vec<ParamDecl> formals;
   pub ConstructorDecl(Str cpp_name, Str header, Node<Type> self, Vec<ParamDecl> formals);
};

/// A c++ class/struct/union method.
class MethodDecl {
   pub const Node<Sym> name;
   pub const Str cpp_name;
   pub const Str header;
   pub const Node<Type> self;
   pub const Vec<ParamDecl> formals;
   pub const Opt<Node<Type>> return_type;
   pub MethodDecl(Str name, Str cpp_name, Str header, Node<Type> self, Vec<ParamDecl> formals,
                  Opt<Node<Type>> return_type);
};

/// Some kind of routine declaration. Should be stored within a Node.
using RoutineDecl = Union<FunctionDecl, ConstructorDecl, MethodDecl>;

/// A variable declaration.
class VariableDecl {
   pub const Node<Sym> name;
   pub const Str cpp_name;
   pub const Str header;
   pub const Node<Type> type;
   pub VariableDecl(Str name, Str cpp_name, Str header, Node<Type> type);
};
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

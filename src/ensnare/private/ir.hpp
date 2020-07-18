#pragma once

#include "ensnare/private/utils.hpp"

#include <cstdint>

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
// This is reference counted string class with a history. It should only be used like `Node<Sym`>.
class Sym {
   priv Vec<Str> detail;
   pub Sym(const Str& name);
   pub fn latest() -> Str;
};

class AtomType;
class PtrType;
class RefType;
class OpaqueType;

using Type = Union<AtomType, PtrType, RefType, OpaqueType>;

// An identifier.
class AtomType {
   pub const Node<Sym> name;
   pub AtomType(const Str& name);
   pub AtomType(const Node<Sym>& name);
};

// Just a raw pointer.
class PtrType {
   pub const Node<Type> pointee;
   pub PtrType(Node<Type> pointee);
};

// A c++ reference pointer.
class RefType {
   pub const Node<Type> pointee;
   pub RefType(Node<Type> pointee);
};

class OpaqueType {};

// A `type Foo = Bar[X, Y]` like declaration.
// These come from c++ declarations like `using Foo = Bar[X, Y];` or `typedef Bar[X, Y] Foo;`
class AliasTypeDecl {
   pub const Node<Sym> name;
   pub const Node<Type> type;
   pub AliasTypeDecl(const Str name, const Node<Type> type);
};

class EnumFieldDecl {
   pub const Node<Sym> name;
   pub const Opt<std::int64_t> val;
   pub EnumFieldDecl(const Str name);
   pub EnumFieldDecl(const Str name, std::int64_t val);
};

// A c++ enum of some kind. It may not be mapped as a nim enum.
class EnumTypeDecl {
   pub const Node<Sym> name;
   pub const Str cpp_name;
   pub const Str header;
   pub const Vec<EnumFieldDecl> fields;
   pub EnumTypeDecl(const Str name, const Str cpp_name, const Str header,
                    const Vec<EnumFieldDecl> fields);
};

// A `struct` / `class` / `union` type declaration.
class RecordTypeDecl {
   pub const Node<Sym> name;
   pub const Str cpp_name;
   pub const Str header;
   pub RecordTypeDecl(const Str name, const Str cpp_name, const Str header);
};

using TypeDecl = Union<AliasTypeDecl, EnumTypeDecl, RecordTypeDecl>;

class ParamDecl {
   pub const Node<Sym> name;
   pub const Node<Type> type;
   // READ_ONLY(Opt<Node<Expr>>, expr);
   pub ParamDecl(const Str name, const Node<Type> type);
};

// A non method function declaration.
class FunctionDecl {
   pub const Node<Sym> name;
   pub const Str cpp_name;
   pub const Str header;
   pub const Vec<ParamDecl> formals;
   pub const Opt<Node<Type>> return_type;
   pub FunctionDecl(const Str name, const Str cpp_name, const Str header,
                    const Vec<ParamDecl> formals, const Opt<Node<Type>> return_type);
};

// A c++ class/struct/union constructor
class ConstructorDecl {
   pub const Str cpp_name;
   pub const Str header;
   pub const Node<Type> self;
   pub const Vec<ParamDecl> formals;
   pub ConstructorDecl(const Str cpp_name, const Str header, const Node<Type> self,
                       const Vec<ParamDecl> formals);
};

// A c++ class/struct/union method
class MethodDecl {
   pub const Node<Sym> name;
   pub const Str cpp_name;
   pub const Str header;
   pub const Node<Type> self;
   pub const Vec<ParamDecl> formals;
   pub const Opt<Node<Type>> return_type;
   pub MethodDecl(const Str name, const Str cpp_name, const Str header, const Node<Type> self,
                  const Vec<ParamDecl> formals, const Opt<Node<Type>> return_type);
};

using RoutineDecl = Union<FunctionDecl, ConstructorDecl, MethodDecl>;

class VariableDecl {
   pub const Node<Sym> name;
   pub const Str cpp_name;
   pub const Str header;
   pub const Node<Type> type;
   pub VariableDecl(const Str name, const Str cpp_name, const Str header, const Node<Type> type);
};
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

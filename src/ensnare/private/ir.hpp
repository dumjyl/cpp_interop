/// \file
/// All of the types that are not Unions appear inside a `Node<_>` or a `Node<Union<_>>`
///
/// This is a symbolized IR. Care must be taken to not produce multiple nodes for the same
/// symbolic entity.

#pragma once

#include "ensnare/private/utils.hpp"

#include <cstdint>

namespace ensnare {
/// This is reference counted string class with a history. Should be stored within a node.
class Sym {
   private:
   Vec<Str> detail;
   bool _no_stropping; ///< This symbol does not require stropping even if it is a keyword.
   public:
   Sym(Str name, bool no_stropping = false);
   void update(Str name);
   Str latest() const;
   bool no_stropping() const;
};

template <typename T> class LitExpr;
class ConstParamExpr;

using Expr = Union<LitExpr<U64>, LitExpr<I64>, ConstParamExpr>;

template <typename T> class LitExpr {
   public:
   const T value;
   LitExpr(T value) : value(value) {}
};

class ConstParamExpr {
   public:
   const Node<Sym> name;
   ConstParamExpr(Node<Sym> name);
};

class PtrType;
class RefType;
class OpaqueType;
class InstType;
class UnsizedArrayType;
class ArrayType;
class FuncType;
class ConstType;

/// Some kind of type. Should be stored within a Node.
using Type = Union<Node<Sym>, PtrType, RefType, OpaqueType, InstType, UnsizedArrayType, ArrayType,
                   FuncType, ConstType>;

/// Just a raw pointer.
class PtrType {
   public:
   const Node<Type> pointee;
   PtrType(Node<Type> pointee);
};

/// A c++ reference pointer.
class RefType {
   public:
   const Node<Type> pointee;
   RefType(Node<Type> pointee);
};

class OpaqueType {};

class InstType {
   public:
   const Node<Sym> name;
   const Vec<Node<Type>> types;
   InstType(Node<Sym> name, Vec<Node<Type>> types);
};

class UnsizedArrayType {
   public:
   const Node<Type> type;
   UnsizedArrayType(Node<Type> type);
};

class ArrayType {
   public:
   const Node<Expr> size;
   const Node<Type> type;
   ArrayType(Node<Expr> size, Node<Type> type);
};

class FuncType {
   public:
   const Vec<Node<Type>> formals;
   const Node<Type> return_type;
   FuncType(Vec<Node<Type>> formals, Node<Type> return_type);
};

class ConstType {
   public:
   const Node<Type> type;
   ConstType(Node<Type> type);
};

/// A `type Foo = Bar[X, Y]` like declaration.
/// These come from c++ declarations like `using Foo = Bar[X, Y];` or `typedef Bar[X, Y] Foo;`
class AliasTypeDecl {
   public:
   const Node<Sym> name;
   const Node<Type> type;
   AliasTypeDecl(Str name, Node<Type> type);
   AliasTypeDecl(Node<Sym> name, Node<Type> type);
};

/// An enum field declation.
class EnumFieldDecl {
   public:
   const Node<Sym> name;
   const Opt<std::int64_t> val;
   EnumFieldDecl(Str name);
   EnumFieldDecl(Str name, std::int64_t val);
};

/// A c++ enum of some kind. It may not be mapped as a nim enum.
class EnumTypeDecl {
   public:
   const Node<Sym> name;
   const Str cpp_name;
   const Str header;
   const Vec<EnumFieldDecl> fields;
   EnumTypeDecl(Str name, Str cpp_name, Str header, Vec<EnumFieldDecl> fields);
   EnumTypeDecl(Node<Sym> name, Str cpp_name, Str header, Vec<EnumFieldDecl> fields);
};

class RecordFieldDecl {
   public:
   const Node<Sym> name;
   const Node<Type> type;
   RecordFieldDecl(Str name, Node<Type> type);
};

/// A `struct` / `class` / `union` type declaration.
class RecordTypeDecl {
   public:
   const Node<Sym> name;
   const Str cpp_name;
   const Str header;
   const Vec<RecordFieldDecl> fields;
   RecordTypeDecl(Str name, Str cpp_name, Str header, Vec<RecordFieldDecl> fields);
   RecordTypeDecl(Node<Sym> name, Str cpp_name, Str header, Vec<RecordFieldDecl> fields);
};

class TemplateParamDecl {
   public:
   const Node<Sym> name;
   const Opt<Node<Type>> constraint;
   TemplateParamDecl(Node<Sym> name);
   TemplateParamDecl(Node<Sym> name, Node<Type> constraint);
};

/// A `struct` / `class` / `union` type declaration.
class TemplateRecordTypeDecl {
   public:
   const Node<Sym> name;
   const Str cpp_name;
   const Str header;
   const Vec<Node<TemplateParamDecl>> generics;
   const Vec<RecordFieldDecl> fields;
   TemplateRecordTypeDecl(Node<Sym> name, Str cpp_name, Str header,
                          Vec<Node<TemplateParamDecl>> generics, Vec<RecordFieldDecl> fields);
};

/// Some kind of type declaration. Should be stored within a Node.
using TypeDecl = Union<AliasTypeDecl, EnumTypeDecl, RecordTypeDecl, TemplateRecordTypeDecl>;

Sym& name(Node<TypeDecl> decl);

/// A routine parameter declaration.
class ParamDecl {
   private:
   Node<Sym> _name;
   Node<Type> _type;

   public:
   Node<Sym> name() const;
   Node<Type> type() const;
   ParamDecl(Str name, Node<Type> type);
   ParamDecl(Node<Sym> name, Node<Type> type);
};

/// A non method function declaration.
class FunctionDecl {
   public:
   const Node<Sym> name;
   const Str cpp_name;
   const Str header;
   const Vec<ParamDecl> formals;
   const Opt<Node<Type>> return_type;
   FunctionDecl(Str name, Str cpp_name, Str header, Vec<ParamDecl> formals,
                Opt<Node<Type>> return_type);
};

/// A c++ class/struct/union constructor.
class ConstructorDecl {
   public:
   const Str cpp_name;
   const Str header;
   const Node<Type> self;
   const Vec<ParamDecl> formals;
   ConstructorDecl(Str cpp_name, Str header, Node<Type> self, Vec<ParamDecl> formals);
};

/// A c++ class/struct/union method.
class MethodDecl {
   public:
   const Node<Sym> name;
   const Str cpp_name;
   const Str header;
   const Node<Type> self;
   const Vec<ParamDecl> formals;
   const Opt<Node<Type>> return_type;
   MethodDecl(Str name, Str cpp_name, Str header, Node<Type> self, Vec<ParamDecl> formals,
              Opt<Node<Type>> return_type);
};

class TemplateFunctionDecl {
   public:
   const Node<Sym> name;
   const Str cpp_name;
   const Str header;
   const Vec<Node<TemplateParamDecl>> generics;
   const Vec<ParamDecl> formals;
   const Opt<Node<Type>> return_type;
   TemplateFunctionDecl(Str name, Str cpp_name, Str header, Vec<Node<TemplateParamDecl>> generics,
                        Vec<ParamDecl> formals, Opt<Node<Type>> return_type);
};

class TemplateConstructorDecl {
   public:
   const Str cpp_name;
   const Str header;
   const Node<Type> self;
   const Vec<Node<TemplateParamDecl>> generics;
   const Vec<ParamDecl> formals;
   TemplateConstructorDecl(Str cpp_name, Str header, Node<Type> self,
                           Vec<Node<TemplateParamDecl>> generics, Vec<ParamDecl> formals);
};

class TemplateMethodDecl {
   public:
   const Node<Sym> name;
   const Str cpp_name;
   const Str header;
   const Node<Type> self;
   const Vec<Node<TemplateParamDecl>> generics;
   const Vec<ParamDecl> formals;
   const Opt<Node<Type>> return_type;
   TemplateMethodDecl(Str name, Str cpp_name, Str header, Node<Type> self,
                      Vec<Node<TemplateParamDecl>> generics, Vec<ParamDecl> formals,
                      Opt<Node<Type>> return_type);
};

/// Some kind of routine declaration. Should be stored within a Node.
using RoutineDecl = Union<FunctionDecl, ConstructorDecl, MethodDecl, TemplateFunctionDecl,
                          TemplateConstructorDecl, TemplateMethodDecl>;

/// A variable declaration.
class VariableDecl {
   public:
   const Node<Sym> name;
   const Str cpp_name;
   const Str header;
   const Node<Type> type;
   VariableDecl(Str name, Str cpp_name, Str header, Node<Type> type);
};
} // namespace ensnare

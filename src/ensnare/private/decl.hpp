#pragma once

#include "ensnare/private/type.hpp"
#include "ensnare/private/utils.hpp"

namespace ensnare {
class AliasTypeDecl {
   public:
   const Sym name;
   const Type type;
   AliasTypeDecl(Sym name, Type type);
};

class EnumFieldDecl {
   public:
   const Sym name;
   const Opt<I64> val;
   EnumFieldDecl(Sym name);
   EnumFieldDecl(Sym name, I64 val);
};

/// FIXME: scoping
class EnumTypeDecl {
   public:
   const Sym name;
   const Str cpp_name;
   const Str header;
   const Vec<EnumFieldDecl> fields;
   EnumTypeDecl(Sym name, Str cpp_name, Str header, Vec<EnumFieldDecl> fields);
};

class RecordFieldDecl {
   public:
   const Sym name;
   const Type type;
   RecordFieldDecl(Str name, Type type);
};

class TemplateParam {
   public:
   const Sym name;
   const Opt<Type> constraint;
   TemplateParam(Sym name);
   TemplateParam(Sym name, Type constraint);
};

Node<TemplateParam> new_TemplateParam(Sym name);

Node<TemplateParam> new_TemplateParam(Sym name, Type constraint);

using TemplateParams = Vec<Node<TemplateParam>>;

class RecordTypeDecl {
   public:
   const Sym name;
   const Str cpp_name;
   const Str header;
   const Opt<TemplateParams> template_params;
   const Vec<RecordFieldDecl> fields;
   RecordTypeDecl(Sym name, Str cpp_name, Str header, Vec<RecordFieldDecl> fields);
   RecordTypeDecl(Sym name, Str cpp_name, Str header, TemplateParams template_params,
                  Vec<RecordFieldDecl> fields);
};

using TypeDeclObj = Union<AliasTypeDecl, EnumTypeDecl, RecordTypeDecl>;
using TypeDecl = Node<TypeDeclObj>;

template <typename T> TypeDecl new_TypeDecl(T type_decl) { return node<TypeDeclObj>(type_decl); }

SymObj& name(TypeDecl decl);

class Param {
   private:
   Sym _name;
   Type _type;
   Opt<Expr> _expr;

   public:
   Sym name() const;
   Type type() const;
   Opt<Expr> expr() const;
   Param(Sym name, Type type);
   Param(Sym name, Type type, Expr expr);
};

using Params = Vec<Param>;

class FunctionDecl {
   public:
   const Sym name;
   const Str cpp_name;
   const Str header;
   const Opt<TemplateParams> template_params;
   const Params params;
   const Opt<Type> return_type;
   FunctionDecl(Str name, Str cpp_name, Str header, Params params, Opt<Type> return_type);
   FunctionDecl(Str name, Str cpp_name, Str header, TemplateParams template_params, Params params,
                Opt<Type> return_type);
};

class ConstructorDecl {
   public:
   const Str cpp_name;
   const Str header;
   const Opt<TemplateParams> self_template_params;
   const Type self;
   const Opt<TemplateParams> template_params;
   const Params params;
   ConstructorDecl(Str cpp_name, Str header, Opt<TemplateParams> self_template_params, Type self,
                   Params params);
   ConstructorDecl(Str cpp_name, Str header, Opt<TemplateParams> self_template_params, Type self,
                   TemplateParams template_params, Params params);
};

class MethodDecl {
   public:
   const Sym name;
   const Str cpp_name;
   const Str header;
   const Opt<TemplateParams> self_template_params;
   const Type self;
   const Opt<TemplateParams> template_params;
   const Params params;
   const Opt<Type> return_type;
   const bool is_static;
   MethodDecl(Str name, Str cpp_name, Str header, Opt<TemplateParams> self_template_params,
              Type self, Params params, Opt<Type> return_type, bool is_static = false);
   MethodDecl(Str name, Str cpp_name, Str header, Opt<TemplateParams> self_template_params,
              Type self, TemplateParams template_params, Params params, Opt<Type> return_type,
              bool is_static = false);
};

using RoutineDeclObj = Union<FunctionDecl, ConstructorDecl, MethodDecl>;
using RoutineDecl = Node<RoutineDeclObj>;

template <typename T> RoutineDecl new_RoutineDecl(T routine_decl) {
   return node<RoutineDeclObj>(routine_decl);
}

class VariableDeclObj {
   public:
   const Sym name;
   const Str cpp_name;
   const Str header;
   const Type type;
   VariableDeclObj(Str name, Str cpp_name, Str header, Type type);
};

using VariableDecl = Node<VariableDeclObj>;

template <typename... Args> VariableDecl new_VariableDecl(Args... args) {
   return node<VariableDeclObj>(args...);
}
} // namespace ensnare

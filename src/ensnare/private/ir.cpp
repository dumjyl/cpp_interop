#include "ensnare/private/ir.hpp"

using namespace ensnare;

ensnare::Sym::Sym(Str name, bool no_stropping) : detail({name}), _no_stropping(no_stropping) {}

void ensnare::Sym::update(Str name) { detail.push_back(name); }

Str ensnare::Sym::latest() const { return detail.back(); }

bool ensnare::Sym::no_stropping() const { return _no_stropping; }

ensnare::ConstParamExpr::ConstParamExpr(Node<Sym> name) : name(name) {}

ensnare::PtrType::PtrType(Node<Type> pointee) : pointee(pointee) {}

ensnare::RefType::RefType(Node<Type> pointee) : pointee(pointee) {}

ensnare::InstType::InstType(Node<Sym> name, Vec<Node<Type>> types) : name(name), types(types) {}

ensnare::UnsizedArrayType::UnsizedArrayType(Node<Type> type) : type(type) {}

ensnare::ArrayType::ArrayType(Node<Expr> size, Node<Type> type) : size(size), type(type) {}

ensnare::FuncType::FuncType(Vec<Node<Type>> formals, Node<Type> return_type)
   : formals(formals), return_type(return_type) {}

ensnare::ConstType::ConstType(Node<Type> type) : type(type) {}

ensnare::AliasTypeDecl::AliasTypeDecl(Str name, Node<Type> type)
   : name(node<Sym>(name)), type(type) {}

ensnare::AliasTypeDecl::AliasTypeDecl(Node<Sym> name, Node<Type> type) : name(name), type(type) {}

ensnare::EnumFieldDecl::EnumFieldDecl(Str name) : name(node<Sym>(name)) {}

ensnare::EnumFieldDecl::EnumFieldDecl(Str name, I64 val) : name(node<Sym>(name)), val(val) {}

ensnare::EnumTypeDecl::EnumTypeDecl(Str name, Str cpp_name, Str header, Vec<EnumFieldDecl> fields)
   : name(node<Sym>(name)), cpp_name(cpp_name), header(header), fields(fields) {}

ensnare::EnumTypeDecl::EnumTypeDecl(Node<Sym> name, Str cpp_name, Str header,
                                    Vec<EnumFieldDecl> fields)
   : name(name), cpp_name(cpp_name), header(header), fields(fields) {}

ensnare::RecordFieldDecl::RecordFieldDecl(Str name, Node<Type> type)
   : name(node<Sym>(name)), type(type) {}

ensnare::RecordTypeDecl::RecordTypeDecl(Str name, Str cpp_name, Str header,
                                        Vec<RecordFieldDecl> fields)
   : name(node<Sym>(name)), cpp_name(cpp_name), header(header), fields(fields) {}

ensnare::RecordTypeDecl::RecordTypeDecl(Node<Sym> name, Str cpp_name, Str header,
                                        Vec<RecordFieldDecl> fields)
   : name(name), cpp_name(cpp_name), header(header), fields(fields) {}

ensnare::TemplateRecordTypeDecl::TemplateRecordTypeDecl(Node<Sym> name, Str cpp_name, Str header,
                                                        Vec<Node<TemplateParamDecl>> generics,
                                                        Vec<RecordFieldDecl> fields)
   : name(name), cpp_name(cpp_name), header(header), generics(generics), fields(fields) {}

Sym& ensnare::name(Node<TypeDecl> decl) {
   if (is<AliasTypeDecl>(decl)) {
      return *as<AliasTypeDecl>(decl).name;
   } else if (is<EnumTypeDecl>(decl)) {
      return *as<EnumTypeDecl>(decl).name;
   } else if (is<RecordTypeDecl>(decl)) {
      return *as<RecordTypeDecl>(decl).name;
   } else if (is<TemplateRecordTypeDecl>(decl)) {
      return *as<TemplateRecordTypeDecl>(decl).name;
   } else {
      fatal("unreachable: render(TypeDecl)");
   }
}

ensnare::ParamDecl::ParamDecl(Str name, Node<Type> type) : _name(node<Sym>(name)), _type(type) {}

ensnare::ParamDecl::ParamDecl(Node<Sym> name, Node<Type> type) : _name(name), _type(type) {}

ensnare::TemplateParamDecl::TemplateParamDecl(Node<Sym> name) : name(name) {}

ensnare::TemplateParamDecl::TemplateParamDecl(Node<Sym> name, Node<Type> constraint)
   : name(name), constraint(constraint) {}

Node<Sym> ensnare::ParamDecl::name() const { return _name; }

Node<Type> ensnare::ParamDecl::type() const { return _type; }

ensnare::FunctionDecl::FunctionDecl(Str name, Str cpp_name, Str header, Vec<ParamDecl> formals,
                                    Opt<Node<Type>> return_type)
   : name(node<Sym>(name)),
     cpp_name(cpp_name),
     header(header),
     formals(formals),
     return_type(return_type) {}

ensnare::ConstructorDecl::ConstructorDecl(Str cpp_name, Str header, Node<Type> self,
                                          Vec<ParamDecl> formals)
   : cpp_name(cpp_name), header(header), self(self), formals(formals) {}

ensnare::MethodDecl::MethodDecl(Str name, Str cpp_name, Str header, Node<Type> self,
                                Vec<ParamDecl> formals, Opt<Node<Type>> return_type)
   : name(node<Sym>(name)),
     cpp_name(cpp_name),
     header(header),
     self(self),
     formals(formals),
     return_type(return_type) {}

ensnare::TemplateFunctionDecl::TemplateFunctionDecl(Str name, Str cpp_name, Str header,
                                                    Vec<Node<TemplateParamDecl>> generics,
                                                    Vec<ParamDecl> formals,
                                                    Opt<Node<Type>> return_type)
   : name(node<Sym>(name)),
     cpp_name(cpp_name),
     header(header),
     generics(generics),
     formals(formals),
     return_type(return_type) {}

ensnare::TemplateConstructorDecl::TemplateConstructorDecl(Str cpp_name, Str header, Node<Type> self,
                                                          Vec<Node<TemplateParamDecl>> generics,
                                                          Vec<ParamDecl> formals)
   : cpp_name(cpp_name), header(header), self(self), generics(generics), formals(formals) {}

ensnare::TemplateMethodDecl::TemplateMethodDecl(Str name, Str cpp_name, Str header, Node<Type> self,
                                                Vec<Node<TemplateParamDecl>> generics,
                                                Vec<ParamDecl> formals, Opt<Node<Type>> return_type)
   : name(node<Sym>(name)),
     cpp_name(cpp_name),
     header(header),
     self(self),
     generics(generics),
     formals(formals),
     return_type(return_type) {}

ensnare::VariableDecl::VariableDecl(Str name, Str cpp_name, Str header, Node<Type> type)
   : name(node<Sym>(name)), cpp_name(cpp_name), header(header), type(type) {}

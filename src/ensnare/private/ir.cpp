#include "ensnare/private/ir.hpp"

// preserve order
#include "ensnare/private/syn.hpp"

ensnare::Sym::Sym(Str name, bool no_stropping) : detail({name}), _no_stropping(no_stropping) {}

void ensnare::Sym::update(Str name) { detail.push_back(name); }

fn ensnare::Sym::latest() const -> Str { return detail.back(); }

fn ensnare::Sym::no_stropping() const -> bool { return _no_stropping; }

ensnare::PtrType::PtrType(Node<Type> pointee) : pointee(pointee) {}

ensnare::RefType::RefType(Node<Type> pointee) : pointee(pointee) {}

ensnare::InstType::InstType(Node<Sym> name, Vec<Node<Type>> types) : name(name), types(types) {}

ensnare::UnsizedArrayType::UnsizedArrayType(Node<Type> type) : type(type) {}

ensnare::ArrayType::ArrayType(std::size_t size, Node<Type> type) : size(size), type(type) {}

ensnare::FuncType::FuncType(Vec<Node<Type>> formals, Node<Type> return_type)
   : formals(formals), return_type(return_type) {}

ensnare::AliasTypeDecl::AliasTypeDecl(Str name, Node<Type> type)
   : name(node<Sym>(name)), type(type) {}

ensnare::AliasTypeDecl::AliasTypeDecl(Node<Sym> name, Node<Type> type) : name(name), type(type) {}

ensnare::EnumFieldDecl::EnumFieldDecl(Str name) : name(node<Sym>(name)) {}

ensnare::EnumFieldDecl::EnumFieldDecl(Str name, std::int64_t val)
   : name(node<Sym>(name)), val(val) {}

ensnare::EnumTypeDecl::EnumTypeDecl(Str name, Str cpp_name, Str header, Vec<EnumFieldDecl> fields)
   : name(node<Sym>(name)), cpp_name(cpp_name), header(header), fields(fields) {}

ensnare::RecordFieldDecl::RecordFieldDecl(Str name, Node<Type> type)
   : name(node<Sym>(name)), type(type) {}

ensnare::RecordTypeDecl::RecordTypeDecl(Str name, Str cpp_name, Str header,
                                        Vec<RecordFieldDecl> fields)
   : name(node<Sym>(name)), cpp_name(cpp_name), header(header), fields(fields) {}

ensnare::RecordTypeDecl::RecordTypeDecl(Node<Sym> name, Str cpp_name, Str header,
                                        Vec<RecordFieldDecl> fields)
   : name(name), cpp_name(cpp_name), header(header), fields(fields) {}

fn ensnare::name(Node<TypeDecl> decl) -> Sym& {
   if (is<AliasTypeDecl>(decl)) {
      return *deref<AliasTypeDecl>(decl).name;
   } else if (is<EnumTypeDecl>(decl)) {
      return *deref<EnumTypeDecl>(decl).name;
   } else if (is<RecordTypeDecl>(decl)) {
      return *deref<RecordTypeDecl>(decl).name;
   } else {
      fatal("unreachable: render(TypeDecl)");
   }
}

ensnare::ParamDecl::ParamDecl(Str name, Node<Type> type) : _name(node<Sym>(name)), _type(type) {}

ensnare::ParamDecl::ParamDecl(Node<Sym> name, Node<Type> type) : _name(name), _type(type) {}

fn ensnare::ParamDecl::name() const -> Node<Sym> { return _name; }

fn ensnare::ParamDecl::type() const -> Node<Type> { return _type; }

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

ensnare::VariableDecl::VariableDecl(Str name, Str cpp_name, Str header, Node<Type> type)
   : name(node<Sym>(name)), cpp_name(cpp_name), header(header), type(type) {}

#include "ensnare/private/undef_syn.hpp"

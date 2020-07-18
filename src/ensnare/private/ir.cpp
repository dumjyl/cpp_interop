#include "ensnare/private/ir.hpp"

// preserve order
#include "ensnare/private/syn.hpp"

ensnare::Sym::Sym(const Str& name) : detail({name}) {}

fn ensnare::Sym::latest() -> Str { return detail.back(); }

ensnare::AtomType::AtomType(const Node<Sym>& name) : name(name) {}

ensnare::AtomType::AtomType(const Str& name) : name(node<Sym>(name)) {}

ensnare::PtrType::PtrType(Node<Type> pointee) : pointee(pointee) {}

ensnare::RefType::RefType(Node<Type> pointee) : pointee(pointee) {}

ensnare::AliasTypeDecl::AliasTypeDecl(const Str name, const Node<Type> type)
   : name(node<Sym>(name)), type(type) {}

ensnare::EnumFieldDecl::EnumFieldDecl(const Str name) : name(node<Sym>(name)) {}

ensnare::EnumFieldDecl::EnumFieldDecl(const Str name, std::int64_t val)
   : name(node<Sym>(name)), val(val) {}

ensnare::EnumTypeDecl::EnumTypeDecl(const Str name, const Str cpp_name, const Str header,
                                    const Vec<EnumFieldDecl> fields)
   : name(node<Sym>(name)), cpp_name(cpp_name), header(header), fields(fields) {}

ensnare::RecordTypeDecl::RecordTypeDecl(const Str name, const Str cpp_name, const Str header)
   : name(node<Sym>(name)), cpp_name(cpp_name), header(header) {}

ensnare::ParamDecl::ParamDecl(const Str name, const Node<Type> type)
   : name(node<Sym>(name)), type(type) {}

ensnare::FunctionDecl::FunctionDecl(const Str name, const Str cpp_name, const Str header,
                                    const Vec<ParamDecl> formals, const Opt<Node<Type>> return_type)
   : name(node<Sym>(name)),
     cpp_name(cpp_name),
     header(header),
     formals(formals),
     return_type(return_type) {}

ensnare::ConstructorDecl::ConstructorDecl(const Str cpp_name, const Str header,
                                          const Node<Type> self, const Vec<ParamDecl> formals)
   : cpp_name(cpp_name), header(header), self(self), formals(formals) {}

ensnare::MethodDecl::MethodDecl(const Str name, const Str cpp_name, const Str header,
                                const Node<Type> self, const Vec<ParamDecl> formals,
                                const Opt<Node<Type>> return_type)
   : name(node<Sym>(name)),
     cpp_name(cpp_name),
     header(header),
     self(self),
     formals(formals),
     return_type(return_type) {}

ensnare::VariableDecl::VariableDecl(const Str name, const Str cpp_name, const Str header,
                                    const Node<Type> type)
   : name(node<Sym>(name)), cpp_name(cpp_name), header(header), type(type) {}

#include "ensnare/private/undef_syn.hpp"

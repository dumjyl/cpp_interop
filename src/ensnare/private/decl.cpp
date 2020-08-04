#include "ensnare/private/decl.hpp"

using namespace ensnare;

ensnare::AliasTypeDecl::AliasTypeDecl(Sym name, Type type) : name(name), type(type) {}

ensnare::EnumFieldDecl::EnumFieldDecl(Sym name) : name(name) {}

ensnare::EnumFieldDecl::EnumFieldDecl(Sym name, I64 val) : name(name), val(val) {}

ensnare::EnumTypeDecl::EnumTypeDecl(Sym name, Str cpp_name, Str header, Vec<EnumFieldDecl> fields)
   : name(name), cpp_name(cpp_name), header(header), fields(fields) {}

ensnare::RecordFieldDecl::RecordFieldDecl(Str name, Type type) : name(new_Sym(name)), type(type) {}

ensnare::RecordTypeDecl::RecordTypeDecl(Sym name, Str cpp_name, Str header,
                                        Vec<RecordFieldDecl> fields)
   : name(name), cpp_name(cpp_name), header(header), fields(fields) {}

ensnare::RecordTypeDecl::RecordTypeDecl(Sym name, Str cpp_name, Str header,
                                        TemplateParams template_params, Vec<RecordFieldDecl> fields)
   : name(name),
     cpp_name(cpp_name),
     header(header),
     template_params(template_params),
     fields(fields) {}

SymObj& ensnare::name(TypeDecl decl) {
   if (is<AliasTypeDecl>(decl)) {
      return *as<AliasTypeDecl>(decl).name;
   } else if (is<EnumTypeDecl>(decl)) {
      return *as<EnumTypeDecl>(decl).name;
   } else if (is<RecordTypeDecl>(decl)) {
      return *as<RecordTypeDecl>(decl).name;
   } else {
      fatal("unreachable: render(TypeDecl)");
   }
}

ensnare::Param::Param(Sym name, Type type) : _name(name), _type(type) {}

ensnare::TemplateParam::TemplateParam(Sym name) : name(name) {}

ensnare::TemplateParam::TemplateParam(Sym name, Type constraint)
   : name(name), constraint(constraint) {}

Node<TemplateParam> ensnare::new_TemplateParam(Sym name) { return node<TemplateParam>(name); }

Node<TemplateParam> ensnare::new_TemplateParam(Sym name, Type constraint) {
   return node<TemplateParam>(name, constraint);
}

Sym ensnare::Param::name() const { return _name; }

Type ensnare::Param::type() const { return _type; }

ensnare::FunctionDecl::FunctionDecl(Str name, Str cpp_name, Str header, Params params,
                                    Opt<Type> return_type)
   : name(new_Sym(name)),
     cpp_name(cpp_name),
     header(header),
     params(params),
     return_type(return_type) {}

ensnare::FunctionDecl::FunctionDecl(Str name, Str cpp_name, Str header,
                                    TemplateParams template_params, Params params,
                                    Opt<Type> return_type)
   : name(new_Sym(name)),
     cpp_name(cpp_name),
     header(header),
     template_params(template_params),
     params(params),
     return_type(return_type) {}

ensnare::ConstructorDecl::ConstructorDecl(Str cpp_name, Str header, Type self, Params params)
   : cpp_name(cpp_name), header(header), self(self), params(params) {}

ensnare::MethodDecl::MethodDecl(Str name, Str cpp_name, Str header, Type self, Params params,
                                Opt<Type> return_type)
   : name(new_Sym(name)),
     cpp_name(cpp_name),
     header(header),
     self(self),
     params(params),
     return_type(return_type) {}

ensnare::VariableDeclObj::VariableDeclObj(Str name, Str cpp_name, Str header, Type type)
   : name(new_Sym(name)), cpp_name(cpp_name), header(header), type(type) {}

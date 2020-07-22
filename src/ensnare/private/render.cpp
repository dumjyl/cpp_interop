#include "ensnare/private/render.hpp"

#include "ensnare/private/str_utils.hpp"

#include "llvm/ADT/StringSet.h"

//
#include "ensnare/private/syn.hpp"

#include <cstdint>

namespace ensnare {
const std::size_t indent_size = 3;
const Node<Sym> anon_name = node<Sym>("�", true);

fn indent() -> Str {
   Str result;
   for (auto i = 0; i < indent_size; i += 1) {
      result += ' ';
   }
   return result;
}

fn indent(const Str& str) -> Str {
   Str result;
   for (const auto line : split_newlines(str)) {
      result += indent() + line + "\n";
   }
   return result;
}

fn render(const Node<Type>& type) -> Str;

const llvm::StringSet nim_keywords(
    {"nil",      "addr",      "asm",      "bind",   "mixin",    "block",     "break",   "do",
     "continue", "defer",     "discard",  "cast",   "if",       "when",      "case",    "of",
     "elif",     "else",      "end",      "except", "import",   "export",    "include", "from",
     "as",       "is",        "isnot",    "div",    "mod",      "in",        "notin",   "not",
     "and",      "or",        "xor",      "shl",    "shr",      "out",       "proc",    "func",
     "method",   "converter", "template", "macro",  "iterator", "interface", "raise",   "return",
     "finally",  "try",       "tuple",    "object", "enum",     "ptr",       "ref",     "distinct",
     "concept",  "static",    "type",     "using",  "const",    "let",       "var",     "for",
     "while",    "yield"});

fn is_nim_keyword(const Sym& sym) -> bool { return nim_keywords.count(sym.latest()) != 0; }

fn needs_stropping(const Sym& sym) { return is_nim_keyword(sym) || !is_ident_chars(sym.latest()); }

fn render(const Node<Sym> sym) -> Str {
   if (!sym->no_stropping() && needs_stropping(*sym)) {
      return "`" + sym->latest() + "`";
   } else {
      return sym->latest();
   }
}

fn render(const AtomType& type) -> Str { return render(type.name); }

fn render(const PtrType& type) -> Str { return "ptr " + render(type.pointee); }

fn render(const RefType& type) -> Str { return "var " + render(type.pointee); }

fn render(const OpaqueType& type) -> Str { return "object"; }

fn render(const InstType& type) -> Str {
   Str result = render(type.name) + "[";
   auto first = true;
   for (const auto& type_param : type.types) {
      if (!first) {
         result += ", ";
      }
      result += render(type_param);
      first = false;
   }
   result += "]";
   return result;
}

fn render(const Node<Type>& type) -> Str {
   if (is<AtomType>(type)) {
      return render(deref<AtomType>(type));
   } else if (is<PtrType>(type)) {
      return render(deref<PtrType>(type));
   } else if (is<RefType>(type)) {
      return render(deref<RefType>(type));
   } else if (is<OpaqueType>(type)) {
      return render(deref<OpaqueType>(type));
   } else if (is<InstType>(type)) {
      return render(deref<InstType>(type));
   } else {
      fatal("unreachable: render(Type)");
   }
}

fn render_pragmas(const Vec<Str>& pragmas) -> Str {
   Str result = "{.";
   for (const auto& pragma : pragmas) {
      if (result.size() > 2) {
         result += ", ";
      }
      result += pragma;
   }
   result += ".}";
   return result;
}

fn render_colon(const Str& a, const Str& b) -> Str { return a + ": " + b; }

fn import_cpp(const Str& pattern) -> Str { return "import_cpp: \"" + pattern + "\""; }

fn header(const Str& header) -> Str { return "header: \"" + header + "\""; }

fn render(const AliasTypeDecl& decl) -> Str {
   return render(decl.name) + "* = " + render(decl.type) + "\n";
}

fn render(const EnumFieldDecl& decl) -> Str {
   if (decl.val) {
      return indent() + render(decl.name) + " = " + std::to_string(*decl.val) + '\n';
   } else {
      return indent() + render(decl.name) + '\n';
   }
}

fn render(const EnumTypeDecl& decl) -> Str {
   Str result = render(decl.name) + "* " +
                render_pragmas({import_cpp(decl.cpp_name), header(decl.header)}) + " = enum\n";
   for (const auto& field : decl.fields) {
      result += render(field);
   }
   return result;
}

fn render(const RecordFieldDecl& decl) -> Str {
   return indent() + render(decl.name) + ": " + render(decl.type) + '\n';
}

fn render(const RecordTypeDecl& decl) -> Str {
   Str result = render(decl.name) + "* " +
                render_pragmas({import_cpp(decl.cpp_name), header(decl.header)}) + " = object\n";
   for (const auto& field : decl.fields) {
      result += render(field);
   }
   return result;
}

fn render(const Node<TypeDecl>& decl) -> Str {
   if (is<AliasTypeDecl>(decl)) {
      return render(deref<AliasTypeDecl>(decl));
   } else if (is<EnumTypeDecl>(decl)) {
      return render(deref<EnumTypeDecl>(decl));
   } else if (is<RecordTypeDecl>(decl)) {
      return render(deref<RecordTypeDecl>(decl));
   } else {
      fatal("unreachable: render(TypeDecl)");
   }
}

fn render(const Vec<ParamDecl>& formals) -> Str {
   Str result = "(";
   for (auto i = 0; i < formals.size(); i += 1) {
      if (i != 0) {
         result += ", ";
      }
      auto name = render(formals[i].name());
      if (name == "") {
         result += anon_name->latest() + std::to_string(i);
      } else {
         result += name;
      }
      result += ": " + render(formals[i].type());
   }
   result += ")";
   return result;
}

fn render(ParamDecl first_formal, Vec<ParamDecl> formals) -> Str {
   Vec<ParamDecl> tmp = {first_formal};
   tmp.insert(tmp.end(), formals.begin(), formals.end());
   return render(tmp);
}

fn self_param(const ConstructorDecl& decl) -> ParamDecl {
   return ParamDecl(anon_name, node<Type>(InstType(node<Sym>("type", true), {decl.self})));
}

fn render(const FunctionDecl& decl) -> Str {
   auto result = "proc " + render(decl.name) + "*" + render(decl.formals);
   if (decl.return_type) {
      result += ": ";
      result += render(*decl.return_type);
   }
   result += "\n" + indent() +
             render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

fn render(const ConstructorDecl& decl) -> Str {
   Str result = "proc `{}`*" + render(self_param(decl), decl.formals) + ": " + render(decl.self) +
                "\n" + indent() +
                render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

fn render(const MethodDecl& decl) -> Str {
   Str result =
       "proc " + render(decl.name) + "*" + render(ParamDecl(anon_name, decl.self), decl.formals);
   if (decl.return_type) {
      result += ": ";
      result += render(*decl.return_type);
   }
   result += "\n" + indent() +
             render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

fn render(const Node<RoutineDecl>& decl) -> Str {
   if (is<FunctionDecl>(decl)) {
      return render(deref<FunctionDecl>(decl));
   } else if (is<ConstructorDecl>(decl)) {
      return render(deref<ConstructorDecl>(decl));
   } else if (is<MethodDecl>(decl)) {
      return render(deref<MethodDecl>(decl));
   } else {
      fatal("unreachable: render(FunctionDecl)");
   }
}

fn render(const Node<VariableDecl>& decl) -> Str {
   return render(decl->name) + "* " +
          render_pragmas({import_cpp(decl->cpp_name), header(decl->header)}) + ": " +
          render(decl->type) + "\n";
}

fn render(const Vec<Node<TypeDecl>>& decls) -> Str {
   if (decls.size() == 0) {
      return "";
   } else {
      Str result = "\n# --- types\n\n";
      result += "type\n";
      for (const auto& decl : decls) {
         result += indent(render(decl));
      }
      return result;
   }
}

fn render(const Vec<Node<RoutineDecl>>& decls) -> Str {
   if (decls.size() == 0) {
      return "";
   } else {
      Str result = "\n# --- routines\n\n";
      for (const auto& decl : decls) {
         result += render(decl);
      }
      return result;
   }
}

fn render(const Vec<Node<VariableDecl>>& decls) -> Str {
   if (decls.size() == 0) {
      return "";
   } else {
      Str result = "\n# --- variables\n\n";
      result += "var\n";
      for (const auto& decl : decls) {
         result += indent(render(decl));
      }
      return result;
   }
}
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

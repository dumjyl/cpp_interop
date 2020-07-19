#include "ensnare/private/rendering.hpp"

#include "ensnare/private/str_utils.hpp"

#include "llvm/ADT/StringSet.h"

//
#include "ensnare/private/syn.hpp"

#include <cstdint>

namespace ensnare {
const std::size_t indent_size = 3;
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

fn is_nim_keyword(const Str& str) -> bool { return nim_keywords.count(str) != 0; }

fn needs_stropping(const Str& sym) { return is_nim_keyword(sym) || !is_ident_chars(sym); }

fn render(const Node<Sym>& sym) -> Str {
   auto result = sym->latest();
   if (needs_stropping(result)) {
      return "`" + result + "`";
   } else {
      return result;
   }
}

fn render(const AtomType& typ) -> Str { return render(typ.name); }

fn render(const PtrType& typ) -> Str { return "ptr " + render(typ.pointee); }

fn render(const RefType& typ) -> Str { return "var " + render(typ.pointee); }

fn render(const OpaqueType& typ) -> Str { return "object"; }

fn render(const Node<Type>& type) -> Str {
   if (is<AtomType>(type)) {
      return render(deref<AtomType>(type));
   } else if (is<PtrType>(type)) {
      return render(deref<PtrType>(type));
   } else if (is<RefType>(type)) {
      return render(deref<RefType>(type));
   } else if (is<OpaqueType>(type)) {
      return render(deref<OpaqueType>(type));
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

fn render(const RecordTypeDecl& decl) -> Str {
   return render(decl.name) + "* " +
          render_pragmas({import_cpp(decl.cpp_name), header(decl.header)}) + " = object\n";
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

fn render(const ParamDecl& decl) -> Str { return render(decl.name) + ": " + render(decl.type); }

fn render(const FunctionDecl& decl) -> Str {
   auto result = "proc " + render(decl.name) + "*(";
   auto first = true;
   for (const auto& formal : decl.formals) {
      if (not first) {
         result += ", ";
      }
      result += render(formal);
      first = false;
   }
   result += ")";
   if (decl.return_type) {
      result += ": ";
      result += render(*decl.return_type);
   }
   result += "\n" + indent() +
             render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

fn render(const ConstructorDecl& decl) -> Str {
   Str result = "proc `{}`*(Self: type[";
   result += render(decl.self) + "]";
   for (const auto& formal : decl.formals) {
      result += ", " + render(formal);
   }
   result += "): " + render(decl.self);
   result += "\n" + indent() +
             render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

fn render(const MethodDecl& decl) -> Str {
   Str result = "proc " + render(decl.name) + "*(self: ";
   result += render(decl.self);
   for (const auto& formal : decl.formals) {
      result += ", " + render(formal);
   }
   result += ")";
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

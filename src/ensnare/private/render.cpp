#include "ensnare/private/render.hpp"

#include "ensnare/private/str_utils.hpp"

#include "llvm/ADT/StringSet.h"

#include <cstdint>

namespace ensnare {
const Size indent_size = 3;
const Sym anon_name = new_Sym("�", true);

Str indent() {
   Str result;
   for (auto i = 0; i < indent_size; i += 1) {
      result += ' ';
   }
   return result;
}

Str indent(const Str& str) {
   Str result;
   for (const auto line : split_newlines(str)) {
      result += indent() + line + "\n";
   }
   return result;
}

Str render(Type type);

const llvm::StringSet nim_keywords(
    {"nil",       "addr",     "asm",      "bind",    "mixin",     "block",    "do",     "break",
     "yield",     "continue", "defer",    "discard", "cast",      "for",      "while",  "if",
     "when",      "case",     "of",       "elif",    "else",      "end",      "except", "import",
     "export",    "include",  "from",     "as",      "is",        "isnot",    "div",    "mod",
     "in",        "notin",    "not",      "and",     "or",        "xor",      "shl",    "shr",
     "out",       "proc",     "func",     "method",  "converter", "template", "macro",  "iterator",
     "interface", "raise",    "return",   "finally", "try",       "tuple",    "object", "enum",
     "ptr",       "ref",      "distinct", "concept", "static",    "type",     "using",  "const",
     "let",       "var"});

Str render(const Sym sym) {
   auto latest = sym->latest();
   auto is_keyword = nim_keywords.count(latest) != 0;
   auto non_ident_char = !is_ident_chars(latest);
   auto underscore_count = 0;
   auto seen_char = false;
   Str result;
   for (auto i = 0; i < latest.size(); i += 1) {
      auto c = latest[i];
      if (c == '_') {
         underscore_count += 1;
      } else {
         if (underscore_count == 1 && seen_char) {
            result += '_';
         } else {
            for (auto j = 0; j < underscore_count; j += 1) {
               result += "�";
            }
         }
         underscore_count = 0;
         seen_char = true;
         result += c;
      }
   }
   for (auto j = 0; j < underscore_count; j += 1) {
      result += "�";
   }
   if (!sym->no_stropping() && (non_ident_char || is_keyword)) {
      return "`" + result + "`";
   } else {
      return result;
   }
}

Str render(Expr expr) {
   if (is<LitExpr<U64>>(expr)) {
      return std::to_string(as<LitExpr<U64>>(expr).value);
   } else if (is<LitExpr<I64>>(expr)) {
      return std::to_string(as<LitExpr<I64>>(expr).value);
   } else if (is<ConstParamExpr>(expr)) {
      return render(as<ConstParamExpr>(expr).name);
   } else {
      fatal("unhandled expr");
   }
}

Str render(const PtrType& type) { return "ptr " + render(type.pointee); }

Str render(const RefType& type) { return "var " + render(type.pointee); }

Str render(const OpaqueType& type) { return "object"; }

Str render(const InstType& type) {
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

Str render(const UnsizedArrayType& type) { return "CppUnsizedArray[" + render(type.type) + "]"; }

Str render(const ArrayType& type) {
   return "array[" + render(type.size) + ", " + render(type.type) + "]";
}

Str render(const FuncType& type) {
   Str result = "proc (";
   for (auto i = 0; i < type.params.size(); i += 1) {
      if (i != 0) {
         result += ", ";
      }
      result += anon_name->latest() + std::to_string(i);
      result += ": " + render(type.params[i]);
   }
   result += "): " + render(type.return_type);
   return result;
}

Str render(const ConstType& type) { return "CppConst[" + render(type.type) + "]"; }

Str render(Type type) {
   if (is<Sym>(type)) {
      return render(as<Sym>(type));
   } else if (is<PtrType>(type)) {
      return render(as<PtrType>(type));
   } else if (is<RefType>(type)) {
      return render(as<RefType>(type));
   } else if (is<OpaqueType>(type)) {
      return render(as<OpaqueType>(type));
   } else if (is<InstType>(type)) {
      return render(as<InstType>(type));
   } else if (is<UnsizedArrayType>(type)) {
      return render(as<UnsizedArrayType>(type));
   } else if (is<ArrayType>(type)) {
      return render(as<ArrayType>(type));
   } else if (is<FuncType>(type)) {
      return render(as<FuncType>(type));
   } else if (is<ConstType>(type)) {
      return render(as<ConstType>(type));
   } else {
      fatal("unreachable: render(Type)");
   }
}

Str render_pragmas(const Vec<Str>& pragmas) {
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

Str render_colon(const Str& a, const Str& b) { return a + ": " + b; }

Str import_cpp(const Str& pattern) { return "import_cpp: \"" + pattern + "\""; }

Str header(const Str& header) { return "header: \"" + header + "\""; }

Str render(const AliasTypeDecl& decl) {
   return render(decl.name) + "* = " + render(decl.type) + "\n";
}

Str render(const EnumFieldDecl& decl) {
   if (decl.val) {
      return indent() + render(decl.name) + " = " + std::to_string(*decl.val) + '\n';
   } else {
      return indent() + render(decl.name) + '\n';
   }
}

Str render(const EnumTypeDecl& decl) {
   Str result = render(decl.name) + "* " +
                render_pragmas({import_cpp(decl.cpp_name), header(decl.header)}) + " = enum\n";
   for (const auto& field : decl.fields) {
      result += render(field);
   }
   return result;
}

Str render(const RecordFieldDecl& decl) {
   return indent() + render(decl.name) + ": " + render(decl.type) + '\n';
}

Str render(const RecordTypeDecl& decl) {
   Str result = render(decl.name) + "* " +
                render_pragmas({import_cpp(decl.cpp_name), header(decl.header)}) + " = object\n";
   for (const auto& field : decl.fields) {
      result += render(field);
   }
   return result;
}

Str render(const TemplateParam& param) {
   Str result = render(param.name);
   if (param.constraint) {
      result += ": " + render(*param.constraint);
   }
   return result;
}

Str render(const TemplateParams& params) {
   Str result = "[";
   for (auto i = 0; i < params.size(); i += 1) {
      if (i != 0) {
         result += "; ";
      }
      result += render(*params[i]);
   }
   result += "]";
   return result;
}

Str render(TypeDecl decl) {
   if (is<AliasTypeDecl>(decl)) {
      return render(as<AliasTypeDecl>(decl));
   } else if (is<EnumTypeDecl>(decl)) {
      return render(as<EnumTypeDecl>(decl));
   } else if (is<RecordTypeDecl>(decl)) {
      return render(as<RecordTypeDecl>(decl));
   } else {
      fatal("unreachable: render(TypeDecl)");
   }
}

template <typename T> Param self_param(const T& decl) {
   return Param(anon_name, new_Type(InstType(new_Sym("type", true), {decl.self})));
}

Str render(const Params& params) {
   Str result = "(";
   for (auto i = 0; i < params.size(); i += 1) {
      if (i != 0) {
         result += ", ";
      }
      auto name = render(params[i].name());
      if (name == "") {
         result += anon_name->latest() + std::to_string(i);
      } else {
         result += name;
      }
      result += ": " + render(params[i].type());
   }
   result += ")";
   return result;
}

Str render(Param first_param, Params params) {
   Params tmp = {first_param};
   tmp.insert(tmp.end(), params.begin(), params.end());
   return render(tmp);
}

Str render(const FunctionDecl& decl) {
   auto result = "proc " + render(decl.name) + "*" + render(decl.params);
   if (decl.return_type) {
      result += ": ";
      result += render(*decl.return_type);
   }
   result += "\n" + indent() +
             render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

Str render(const ConstructorDecl& decl) {
   Str result = "proc `{}`*" + render(self_param(decl), decl.params) + ": " + render(decl.self) +
                "\n" + indent() +
                render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

Str render(const MethodDecl& decl) {
   Str result =
       "proc " + render(decl.name) + "*" + render(Param(anon_name, decl.self), decl.params);
   if (decl.return_type) {
      result += ": ";
      result += render(*decl.return_type);
   }
   result += "\n" + indent() +
             render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

Str render(const RoutineDecl& decl) {
   if (is<FunctionDecl>(decl)) {
      return render(as<FunctionDecl>(decl));
   } else if (is<ConstructorDecl>(decl)) {
      return render(as<ConstructorDecl>(decl));
   } else if (is<MethodDecl>(decl)) {
      return render(as<MethodDecl>(decl));
   } else {
      fatal("unreachable: render(FunctionDecl)");
   }
}

Str render(const VariableDecl& decl) {
   return render(decl->name) + "* " +
          render_pragmas({import_cpp(decl->cpp_name), header(decl->header)}) + ": " +
          render(decl->type) + "\n";
}

Str render(const Vec<TypeDecl>& decls) {
   if (decls.size() == 0) {
      return "";
   } else {
      Str result = "\ntype\n";
      for (const auto& decl : decls) {
         result += indent(render(decl));
      }
      return result;
   }
}

Str render(const Vec<RoutineDecl>& decls) {
   if (decls.size() == 0) {
      return "";
   } else {
      Str result = "\n";
      for (const auto& decl : decls) {
         result += render(decl);
      }
      return result;
   }
}

Str render(const Vec<VariableDecl>& decls) {
   if (decls.size() == 0) {
      return "";
   } else {
      Str result = "\nvar\n";
      for (const auto& decl : decls) {
         result += indent(render(decl));
      }
      return result;
   }
}
} // namespace ensnare

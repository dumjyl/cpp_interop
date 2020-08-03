#include "ensnare/private/render.hpp"

#include "ensnare/private/str_utils.hpp"

#include "llvm/ADT/StringSet.h"

#include <cstdint>

namespace ensnare {
const std::size_t indent_size = 3;
const Node<Sym> anon_name = node<Sym>("�", true);

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

Str render(const Node<Type>& type);

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

Str render(const Node<Sym> sym) {
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

Str render(Node<Expr> expr) {
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
   for (auto i = 0; i < type.formals.size(); i += 1) {
      if (i != 0) {
         result += ", ";
      }
      result += anon_name->latest() + std::to_string(i);
      result += ": " + render(type.formals[i]);
   }
   result += "): " + render(type.return_type);
   return result;
}

Str render(const ConstType& type) { return "CppConst[" + render(type.type) + "]"; }

Str render(const Node<Type>& type) {
   if (is<Node<Sym>>(type)) {
      return render(as<Node<Sym>>(type));
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

Str render(const TemplateParamDecl& param) {
   Str result = render(param.name);
   if (param.constraint) {
      result += ": " + render(*param.constraint);
   }
   return result;
}

Str render(Vec<Node<TemplateParamDecl>> generics) {
   Str result = "[";
   for (auto i = 0; i < generics.size(); i += 1) {
      if (i != 0) {
         result += "; ";
      }
      result += render(*generics[i]);
   }
   result += "]";
   return result;
}

Str render(const TemplateRecordTypeDecl& decl) {
   Str result = render(decl.name) + "* " + render(decl.generics) + " " +
                render_pragmas({import_cpp(decl.cpp_name), header(decl.header)}) + " = object\n";
   for (const auto& field : decl.fields) {
      result += render(field);
   }
   return result;
}

Str render(const Node<TypeDecl>& decl) {
   if (is<AliasTypeDecl>(decl)) {
      return render(as<AliasTypeDecl>(decl));
   } else if (is<EnumTypeDecl>(decl)) {
      return render(as<EnumTypeDecl>(decl));
   } else if (is<RecordTypeDecl>(decl)) {
      return render(as<RecordTypeDecl>(decl));
   } else if (is<TemplateRecordTypeDecl>(decl)) {
      return render(as<TemplateRecordTypeDecl>(decl));
   } else {
      fatal("unreachable: render(TypeDecl)");
   }
}

template <typename T> ParamDecl self_param(const T& decl) {
   return ParamDecl(anon_name, node<Type>(InstType(node<Sym>("type", true), {decl.self})));
}

Str render(const Vec<ParamDecl>& formals) {
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

Str render(ParamDecl first_formal, Vec<ParamDecl> formals) {
   Vec<ParamDecl> tmp = {first_formal};
   tmp.insert(tmp.end(), formals.begin(), formals.end());
   return render(tmp);
}

Str render(const FunctionDecl& decl) {
   auto result = "proc " + render(decl.name) + "*" + render(decl.formals);
   if (decl.return_type) {
      result += ": ";
      result += render(*decl.return_type);
   }
   result += "\n" + indent() +
             render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

Str render(const ConstructorDecl& decl) {
   Str result = "proc `{}`*" + render(self_param(decl), decl.formals) + ": " + render(decl.self) +
                "\n" + indent() +
                render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

Str render(const MethodDecl& decl) {
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

Str render(const TemplateFunctionDecl& decl) {
   auto result = "proc " + render(decl.name) + "*" + render(decl.generics) + render(decl.formals);
   if (decl.return_type) {
      result += ": ";
      result += render(*decl.return_type);
   }
   result += "\n" + indent() +
             render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

Str render(const TemplateConstructorDecl& decl) {
   Str result = "proc `{}`*" + render(decl.generics) + render(self_param(decl), decl.formals) +
                ": " + render(decl.self) + "\n" + indent() +
                render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

Str render(const TemplateMethodDecl& decl) {
   Str result = "proc " + render(decl.name) + "*" + render(decl.generics) +
                render(ParamDecl(anon_name, decl.self), decl.formals);
   if (decl.return_type) {
      result += ": ";
      result += render(*decl.return_type);
   }
   result += "\n" + indent() +
             render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   return result;
}

Str render(const Node<RoutineDecl>& decl) {
   if (is<FunctionDecl>(decl)) {
      return render(as<FunctionDecl>(decl));
   } else if (is<ConstructorDecl>(decl)) {
      return render(as<ConstructorDecl>(decl));
   } else if (is<MethodDecl>(decl)) {
      return render(as<MethodDecl>(decl));
   } else if (is<TemplateFunctionDecl>(decl)) {
      return render(as<TemplateFunctionDecl>(decl));
   } else if (is<TemplateConstructorDecl>(decl)) {
      return render(as<TemplateConstructorDecl>(decl));
   } else if (is<TemplateMethodDecl>(decl)) {
      return render(as<TemplateMethodDecl>(decl));
   } else {
      fatal("unreachable: render(FunctionDecl)");
   }
}

Str render(const Node<VariableDecl>& decl) {
   return render(decl->name) + "* " +
          render_pragmas({import_cpp(decl->cpp_name), header(decl->header)}) + ": " +
          render(decl->type) + "\n";
}

Str render(const Vec<Node<TypeDecl>>& decls) {
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

Str render(const Vec<Node<RoutineDecl>>& decls) {
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

Str render(const Vec<Node<VariableDecl>>& decls) {
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

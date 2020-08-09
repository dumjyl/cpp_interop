// This isn't just for rendering?
//
// Producing template functions:
// Nim's import_cpp patterns can only reference concrete parameters not template arguments,
// which is what we really want. Fixing this isn't trivial. I tried.
// This only matters for template parameters which cannot be inferred from arguments.
// Instead we (ab)use typedesc parameters which are considered parameters. Consider:
// template <typename T> T init(int n);
// For which we produce:
// proc init_internal[T](n: int, �: type[T]) {.import_cpp: "init<'1>(@)".}
// proc init*[T](n: int) = init_internal(n, T)

#include "ensnare/private/render.hpp"

#include "ensnare/private/str_utils.hpp"

#include "llvm/ADT/StringSet.h"

#include <cstdint>

namespace ensnare {
using std::to_string;
const Size indent_size = 3;
const Str anon_str = "�";
const Sym anon_sym = new_Sym(anon_str, true);

Str indent() {
   Str result;
   for (auto _ : range(indent_size - 1)) {
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
   for (auto i : range(latest.size() - 1)) {
      auto c = latest[i];
      if (c == '_') {
         underscore_count += 1;
      } else {
         if (underscore_count == 1 && seen_char) {
            result += '_';
         } else {
            for (auto j = 0; j < underscore_count; j += 1) {
               result += anon_str;
            }
         }
         underscore_count = 0;
         seen_char = true;
         result += c;
      }
   }
   for (auto _ : range(underscore_count - 1)) {
      result += anon_str;
   }
   if (!sym->no_stropping() && (non_ident_char || is_keyword)) {
      return "`" + result + "`";
   } else {
      return result;
   }
}

Str render(Expr expr) {
   if (is<LitExpr<U64>>(expr)) {
      return to_string(as<LitExpr<U64>>(expr).value);
   } else if (is<LitExpr<I64>>(expr)) {
      return to_string(as<LitExpr<I64>>(expr).value);
   } else if (is<ConstParamExpr>(expr)) {
      return render(as<ConstParamExpr>(expr).name);
   } else {
      fatal("unhandled expr");
   }
}

Str render(const PtrType& type) { return "ptr " + render(type.pointee); }

Str render(const RefType& type) { return "var " + render(type.pointee); }

Str render(const OpaqueType& type) { return "object"; }

Str render(const InstType::Arg& arg) { return visit(LAMBDA(render), arg); }

Str render(const InstType& type) {
   Str result = render(type.type) + "[";
   auto first = true;
   for (const auto& arg : type.args) {
      if (!first) {
         result += ", ";
      }
      result += render(arg);
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
   for (auto i : range(type.params.size() - 1)) {
      if (i != 0) {
         result += ", ";
      }
      result += anon_str + to_string(i);
      result += ": " + render(type.params[i]);
   }
   result += ")";
   if (type.return_type) {
      result += ": " + render(*type.return_type);
   }
   return result;
}

Str render(const ConstType& type) { return "CppConst[" + render(type.type) + "]"; }

Str render(Type type) { return visit(LAMBDA(render), *type); }

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
      return indent() + render(decl.name) + " = " + to_string(*decl.val) + '\n';
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

Str render(const TemplateParam& param) {
   Str result = render(param.name);
   if (param.constraint) {
      result += ": " + render(*param.constraint);
   }
   return result;
}

Opt<Vec<Str>> render(const Opt<TemplateParams>& params) {
   if (params) {
      Vec<Str> result;
      for (auto& param : *params) {
         result.push_back(render(*param));
      }
      return result;
   } else {
      return {};
   }
}

Str render_template_params(const Vec<Str>& params) {
   Str result = "[";
   auto first = true;
   for (auto& param : params) {
      if (!first) {
         result += "; ";
      }
      first = false;
      result += param;
   }
   result += "]";
   return result;
}

Str render(const RecordTypeDecl& decl) {
   Str result = render(decl.name) + "* ";
   if (auto template_params = render(decl.template_params)) {
      result += render_template_params(*template_params) + " ";
   }
   result += render_pragmas({import_cpp(decl.cpp_name), header(decl.header)}) + " = object\n";
   for (const auto& field : decl.fields) {
      result += render(field);
   }
   return result;
}

Str render(TypeDecl decl) { return visit(LAMBDA(render), *decl); }

Type typedesc(Type type) { return new_Type(InstType(new_Type(new_Sym("type", true)), {type})); }

template <typename T> Param type_self_param(const T& decl) {
   return Param(anon_sym, typedesc(decl.self));
}

Str render(const Param& param, int i) {
   Str result;
   auto name = render(param.name());
   if (name == "") {
      result += anon_str + to_string(i);
   } else {
      result += name;
   }
   result += ": " + render(param.type());
   if (param.expr()) {
      result += " = " + render(*param.expr());
   }
   return result;
}

Str render_routine_sig(const Str& name, const Opt<Vec<Str>>& template_params,
                       const Vec<Str>& params, const Opt<Str>& return_type, bool exported) {
   auto result = "proc " + name;
   if (exported) {
      result += "*";
   }
   if (template_params) {
      result += "[";
      auto first = true;
      for (auto& template_param : *template_params) {
         if (!first) {
            result += "; ";
         }
         first = false;
         result += template_param;
      }
      result += "]";
   }
   result += "(";
   auto first = true;
   for (auto& param : params) {
      if (!first) {
         result += ", ";
      }
      first = false;
      result += param;
   }
   result += ")";
   if (return_type) {
      result += ": " + *return_type;
   }
   return result;
}

Vec<Str> render(const Params& params) {
   Vec<Str> result;
   for (auto i : range(params.size() - 1)) {
      result.push_back(render(params[i], i));
   }
   return result;
}

template <typename T> Str render_pragmas(const T& decl) {
   return render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)});
}

template <typename T> Str import_cpp_templ_args(const T& decl) {
   Str result = "<";
   auto first = true;
   for (auto i : range(decl.template_params->size() - 1)) {
      if (!first) {
         result += ", ";
      }
      first = false;
      result += "'" + to_string(decl.params.size() + i + 1);
   }
   result += ">";
   return result;
}

template <typename T> Str render_templ_pragmas(const T& decl) {
   return render_pragmas(
       {import_cpp(decl.cpp_name + import_cpp_templ_args(decl) + "(@)"), header(decl.header)});
}

Opt<Str> render(const Opt<Type>& type) { return type ? render(*type) : Opt<Str>(); }

Str internal_name(Sym sym) { return sym->latest() + "_internal"; }

Params templ_params(const FunctionDecl& decl) {
   Params result = decl.params;
   auto&& template_params = *decl.template_params;
   for (auto i : range(template_params.size() - 1)) {
      result.push_back(Param(new_Sym(anon_str + to_string(decl.params.size() + i + 1), true),
                             typedesc(new_Type(template_params[i]->name))));
   }
   return result;
}

Str render_templ_internal(const FunctionDecl& decl) {
   return render_routine_sig(internal_name(decl.name), render(decl.template_params),
                             render(templ_params(decl)), render(decl.return_type), false) +
          "\n" + indent() + render_templ_pragmas(decl) + "\n";
}

Str forward_templ_call(const FunctionDecl& decl) {
   Str result = internal_name(decl.name) + "(";
   auto first = true;
   for (auto i : range(decl.params.size() - 1)) {
      if (!first) {
         result += ", ";
      }
      first = false;
      result += render(decl.params[i].name());
   }
   for (auto i : range(decl.template_params->size() - 1)) {
      if (!first) {
         result += ", ";
      }
      first = false;
      result += render((*decl.template_params)[i]->name);
   }
   result += ")";
   return result;
}

Str render_templ(const FunctionDecl& decl) {
   return render_routine_sig(render(decl.name), render(decl.template_params), render(decl.params),
                             render(decl.return_type), true) +
          " =\n" + indent() + forward_templ_call(decl) + "\n";
}

Str render(const FunctionDecl& decl) {
   if (decl.template_params) {
      return render_templ_internal(decl) + render_templ(decl);
   } else {
      return render_routine_sig(render(decl.name), {}, render(decl.params),
                                render(decl.return_type), true) +
             "\n" + indent() + render_pragmas(decl) + "\n";
   }
}

Params concat(const Param& param, const Params& params) {
   Params result = {param};
   result.insert(result.end(), params.begin(), params.end());
   return result;
}

Opt<TemplateParams> concat(const Opt<TemplateParams>& a, const Opt<TemplateParams>& b) {
   if (a || b) {
      TemplateParams result;
      if (a) {
         result.insert(result.end(), a->begin(), a->end());
      }
      if (b) {
         result.insert(result.end(), b->begin(), b->end());
      }
      return result;
   } else {
      return {};
   }
}

Str render(const ConstructorDecl& decl) {
   // We don't have to do the template parameter swizzling because c++ template constructors
   // template arguments must be inferrable.
   return render_routine_sig(
              "`{}`", render(concat(decl.self_template_params, decl.template_params)),
              render(concat(type_self_param(decl), decl.params)), render(decl.self), true) +
          "\n" + indent() + render_pragmas(decl) + "\n";
}

Str render_templ_internal(const MethodDecl& decl) { return ""; }

Str render_templ(const MethodDecl& decl) { return ""; }

Str render(const MethodDecl& decl) {
   if (decl.template_params) {
      return render_templ_internal(decl) + render_templ(decl);
   } else {
      return render_routine_sig(
                 render(decl.name), render(decl.self_template_params),
                 render(concat(decl.is_static ? type_self_param(decl) : Param(anon_sym, decl.self),
                               decl.params)),
                 render(decl.return_type), true) +
             "\n" + indent() +
             render_pragmas({import_cpp(decl.cpp_name + "(@)"), header(decl.header)}) + "\n";
   }
}

Str render(const RoutineDecl& decl) { return visit(LAMBDA(render), *decl); }

Str render(const VariableDecl& decl) {
   return render(decl->name) + "* " +
          render_pragmas({import_cpp(decl->cpp_name), header(decl->header)}) + ": " +
          render(decl->type) + "\n";
}

template <typename T> Str render_decls(const Vec<T>& decls, Str&& init, bool needs_indent) {
   if (decls.size() == 0) {
      return "";
   } else {
      Str result = init;
      for (const auto& decl : decls) {
         result += needs_indent ? indent(render(decl)) : render(decl);
      }
      return result;
   }
}

Str render(const Vec<TypeDecl>& decls) { return render_decls(decls, "\ntype\n", true); }

Str render(const Vec<RoutineDecl>& decls) { return render_decls(decls, "\n", false); }

Str render(const Vec<VariableDecl>& decls) { return render_decls(decls, "\nvar\n", true); }
} // namespace ensnare

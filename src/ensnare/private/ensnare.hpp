#pragma once

#include "ensnare/private/bit_utils.hpp"
#include "ensnare/private/headers.hpp"
#include "ensnare/private/ir.hpp"
#include "ensnare/private/os_utils.hpp"
#include "ensnare/private/runtime.hpp"
#include "ensnare/private/str_utils.hpp"
#include "ensnare/private/utils.hpp"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/CommandLine.h"

// preserve order
#include "ensnare/private/syn.hpp"

namespace ensnare {
namespace cl {
namespace cl = llvm::cl;
cl::opt<bool> disable_includes("disable-includes",
                               cl::desc("do not gather system includes. FIXME: not implimented"));
cl::opt<Str> output(cl::Positional, cl::desc("output wrapper name/path"));
cl::list<Str> args(cl::ConsumeAfter, cl::desc("clang args..."));
} // namespace cl

class Config {
   READ_ONLY(Vec<Str>, headers);
   READ_ONLY(Str, output);
   READ_ONLY(Vec<Str>, clang_args);

   pub Config(int argc, const char* argv[]) {
      llvm::cl::ParseCommandLineOptions(argc, argv);
      _output = cl::output;
      for (const auto& arg : cl::args) {
         if (os::is_cpp_file(arg)) {
            _headers.push_back(arg);
         } else {
            _clang_args.push_back(arg);
         }
      }
   };

   // A header file with all the headers added together.
   pub fn header_file() const -> Str {
      if (_headers.size() == 0) {
         fatal("no headers given");
      } else {
         Str result;
         for (const auto& header : _headers) {
            result += "#include ";
            if (os::is_system_header(header)) {
               result += header;
            } else {
               result += "\"" + header + "\"";
            }
            result += '\n';
         }
         return result;
      };
   }
};

namespace rendering {
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
} // namespace rendering

// The cananonical types defined in runtime.nim
class Builtins {
   priv static fn init(const char* name) -> Node<Type> { return node<Type>(AtomType(name)); }

   pub const Node<Type> _schar = Builtins::init("CppSChar");
   pub const Node<Type> _short = Builtins::init("CppShort");
   pub const Node<Type> _int = Builtins::init("CppInt");
   pub const Node<Type> _long = Builtins::init("CppLong");
   pub const Node<Type> _long_long = Builtins::init("CppLongLong");
   pub const Node<Type> _uchar = Builtins::init("CppUChar");
   pub const Node<Type> _ushort = Builtins::init("CppUShort");
   pub const Node<Type> _uint = Builtins::init("CppUInt");
   pub const Node<Type> _ulong = Builtins::init("CppULong");
   pub const Node<Type> _ulong_long = Builtins::init("CppULongLong");
   pub const Node<Type> _char = Builtins::init("CppChar");
   pub const Node<Type> _wchar = Builtins::init("CppWChar");
   pub const Node<Type> _char8 = Builtins::init("CppChar8");
   pub const Node<Type> _char16 = Builtins::init("CppChar16");
   pub const Node<Type> _char32 = Builtins::init("CppChar32");
   pub const Node<Type> _bool = Builtins::init("CppBool");
   pub const Node<Type> _float = Builtins::init("CppFloat");
   pub const Node<Type> _double = Builtins::init("CppDouble");
   pub const Node<Type> _long_double = Builtins::init("CppLongDouble");
   pub const Node<Type> _void = Builtins::init("CppVoid");
   pub const Node<Type> _int128 = Builtins::init("CppInt128");
   pub const Node<Type> _uint128 = Builtins::init("CppUInt128");
   pub const Node<Type> _neon_float16 = Builtins::init("CppNeonFloat16");
   pub const Node<Type> _ocl_float16 = Builtins::init("CppOCLFloat16");
   pub const Node<Type> _float16 = Builtins::init("CppFloat16");
   pub const Node<Type> _float128 = Builtins::init("CppFloat128");
   // we treat cstddef as builtins too.
   pub const Node<Type> _size = Builtins::init("CppSize");
   pub const Node<Type> _ptrdiff = Builtins::init("CppPtrDiff");
   pub const Node<Type> _max_align = Builtins::init("CppMaxAlign");
   pub const Node<Type> _byte = Builtins::init("CppByte");
   pub const Node<Type> _nullptr = Builtins::init("CppNullPtr");

   pub Builtins() {}
};

// A `Context` must not outlive a `Config`
class Context {
   pub const clang::ASTContext& ast_ctx;
   priv Map<const clang::Decl*, Node<Type>>
       type_lookup;                            // For mapping bound declarations to exisiting types.
   READ_ONLY(Vec<Node<TypeDecl>>, type_decls); // Types to output.
   READ_ONLY(Vec<Node<RoutineDecl>>, routine_decls);   // Functions to output.
   READ_ONLY(Vec<Node<VariableDecl>>, variable_decls); // Variables to output.

   pub const Config& cfg;
   pub const Builtins builtins;
   pub Context(const Config& cfg, const clang::ASTContext& ast_ctx) : cfg(cfg), ast_ctx(ast_ctx) {}

   pub fn filename(const clang::SourceLocation& loc) const -> llvm::StringRef {
      return ast_ctx.getSourceManager().getFilename(loc);
   }

   pub fn lookup(const clang::Decl& decl) const -> Opt<Node<Type>> {
      auto decl_type = type_lookup.find(&decl);
      if (decl_type == type_lookup.end()) {
         return {};
      } else {
         return decl_type->second;
      }
   }

   pub fn set(const clang::Decl& decl, const Node<Type> type) {
      auto maybe_type = lookup(decl);
      if (maybe_type) {
         fatal("FIXME: duplicate set");
      } else {
         type_lookup[&decl] = type;
      }
   }

   pub fn add(const Node<TypeDecl> decl) { _type_decls.push_back(decl); }

   pub fn add(const Node<RoutineDecl> decl) { _routine_decls.push_back(decl); }

   pub fn add(const Node<VariableDecl> decl) { _variable_decls.push_back(decl); }

   // Guards from wrapping private declarations.
   pub fn access_filter(const clang::Decl& decl) const -> bool {
      // AS_protected
      // AS_private
      // AS_none
      return decl.getAccess() == clang::AS_public || decl.getAccess() == clang::AS_none;
   }

   pub fn header(const clang::NamedDecl& decl) const -> Opt<Str> {
      for (auto const& header : cfg.headers()) {
         if (ends_with(filename(decl.getLocation()), header)) {
            return header;
         }
      }
      return {};
   }
};

// We have the `wrap` operation which add declarative bindings to the
//`Context` from the `Stmt`s
// we / visit. / We have the `map` operation which produces a suitable ir type
// from a `Type`,
//`QualType` or / even some kinds of `Stmt` (a decltype for example).
//

#define WRAP(T) fn wrap(Context& ctx, const clang::T##Decl& decl) -> void

#define MAP(T) fn map(Context& ctx, T entity) -> Node<Type>

fn wrap(Context& ctx, const clang::Decl& decl) -> void;
MAP(const clang::Decl&);
MAP(const clang::Type&);

template <typename T> fn map(Context& ctx, const T* entity) {
   if (entity) {
      return map(ctx, *entity);
   } else {
      fatal("tried to map a null entity");
   }
}

// Maps to an atomic type of some kind. Many of these are obscure and
// unsupported.
MAP(const clang::BuiltinType&) {
   switch (entity.getKind()) {
      case clang::BuiltinType::Kind::OCLImage1dRO:
      case clang::BuiltinType::Kind::OCLImage1dArrayRO:
      case clang::BuiltinType::Kind::OCLImage1dBufferRO:
      case clang::BuiltinType::Kind::OCLImage2dRO:
      case clang::BuiltinType::Kind::OCLImage2dArrayRO:
      case clang::BuiltinType::Kind::OCLImage2dDepthRO:
      case clang::BuiltinType::Kind::OCLImage2dArrayDepthRO:
      case clang::BuiltinType::Kind::OCLImage2dMSAARO:
      case clang::BuiltinType::Kind::OCLImage2dArrayMSAARO:
      case clang::BuiltinType::Kind::OCLImage2dMSAADepthRO:
      case clang::BuiltinType::Kind::OCLImage2dArrayMSAADepthRO:
      case clang::BuiltinType::Kind::OCLImage3dRO:
      case clang::BuiltinType::Kind::OCLImage1dWO:
      case clang::BuiltinType::Kind::OCLImage1dArrayWO:
      case clang::BuiltinType::Kind::OCLImage1dBufferWO:
      case clang::BuiltinType::Kind::OCLImage2dWO:
      case clang::BuiltinType::Kind::OCLImage2dArrayWO:
      case clang::BuiltinType::Kind::OCLImage2dDepthWO:
      case clang::BuiltinType::Kind::OCLImage2dArrayDepthWO:
      case clang::BuiltinType::Kind::OCLImage2dMSAAWO:
      case clang::BuiltinType::Kind::OCLImage2dArrayMSAAWO:
      case clang::BuiltinType::Kind::OCLImage2dMSAADepthWO:
      case clang::BuiltinType::Kind::OCLImage2dArrayMSAADepthWO:
      case clang::BuiltinType::Kind::OCLImage3dWO:
      case clang::BuiltinType::Kind::OCLImage1dRW:
      case clang::BuiltinType::Kind::OCLImage1dArrayRW:
      case clang::BuiltinType::Kind::OCLImage1dBufferRW:
      case clang::BuiltinType::Kind::OCLImage2dRW:
      case clang::BuiltinType::Kind::OCLImage2dArrayRW:
      case clang::BuiltinType::Kind::OCLImage2dDepthRW:
      case clang::BuiltinType::Kind::OCLImage2dArrayDepthRW:
      case clang::BuiltinType::Kind::OCLImage2dMSAARW:
      case clang::BuiltinType::Kind::OCLImage2dArrayMSAARW:
      case clang::BuiltinType::Kind::OCLImage2dMSAADepthRW:
      case clang::BuiltinType::Kind::OCLImage2dArrayMSAADepthRW:
      case clang::BuiltinType::Kind::OCLImage3dRW:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCMcePayload:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCImePayload:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCRefPayload:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCSicPayload:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCMceResult:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCImeResult:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCRefResult:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCSicResult:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCImeResultSingleRefStreamout:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCImeResultDualRefStreamout:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCImeSingleRefStreamin:
      case clang::BuiltinType::Kind::OCLIntelSubgroupAVCImeDualRefStreamin:
      case clang::BuiltinType::Kind::OCLSampler:
      case clang::BuiltinType::Kind::OCLEvent:
      case clang::BuiltinType::Kind::OCLClkEvent:
      case clang::BuiltinType::Kind::OCLQueue:
      case clang::BuiltinType::Kind::OCLReserveID: {
         fatal("opencl builtins unsupported");
      }
      // --- SVE
      case clang::BuiltinType::Kind::SveInt8:
      case clang::BuiltinType::Kind::SveInt16:
      case clang::BuiltinType::Kind::SveInt32:
      case clang::BuiltinType::Kind::SveInt64:
      case clang::BuiltinType::Kind::SveUint8:
      case clang::BuiltinType::Kind::SveUint16:
      case clang::BuiltinType::Kind::SveUint32:
      case clang::BuiltinType::Kind::SveUint64:
      case clang::BuiltinType::Kind::SveFloat16:
      case clang::BuiltinType::Kind::SveFloat32:
      case clang::BuiltinType::Kind::SveFloat64:
      case clang::BuiltinType::Kind::SveBool: {
         fatal("SVE builtins unsupported");
      }
      case clang::BuiltinType::Kind::ShortAccum:
      case clang::BuiltinType::Kind::Accum:
      case clang::BuiltinType::Kind::LongAccum:
      case clang::BuiltinType::Kind::UShortAccum:
      case clang::BuiltinType::Kind::UAccum:
      case clang::BuiltinType::Kind::ULongAccum:
      case clang::BuiltinType::Kind::ShortFract:
      case clang::BuiltinType::Kind::Fract:
      case clang::BuiltinType::Kind::LongFract:
      case clang::BuiltinType::Kind::UShortFract:
      case clang::BuiltinType::Kind::UFract:
      case clang::BuiltinType::Kind::ULongFract:
      case clang::BuiltinType::Kind::SatShortAccum:
      case clang::BuiltinType::Kind::SatAccum:
      case clang::BuiltinType::Kind::SatLongAccum:
      case clang::BuiltinType::Kind::SatUShortAccum:
      case clang::BuiltinType::Kind::SatUAccum:
      case clang::BuiltinType::Kind::SatULongAccum:
      case clang::BuiltinType::Kind::SatShortFract:
      case clang::BuiltinType::Kind::SatFract:
      case clang::BuiltinType::Kind::SatLongFract:
      case clang::BuiltinType::Kind::SatUShortFract:
      case clang::BuiltinType::Kind::SatUFract:
      case clang::BuiltinType::Kind::SatULongFract: {
         fatal("Sat, Fract, and  Accum builtins unsupported");
      }
      case clang::BuiltinType::Kind::ObjCId:
      case clang::BuiltinType::Kind::ObjCClass:
      case clang::BuiltinType::Kind::ObjCSel: {
         fatal("obj-c builtins unsupported");
      }
      case clang::BuiltinType::Kind::Char_S: { // 'char' for targets where it's
                                               // signed
         fatal("this should be unreachable because nim compiles with unsigned "
               "chars");
      }
      case clang::BuiltinType::Kind::Dependent:
      case clang::BuiltinType::Kind::Overload:
      case clang::BuiltinType::Kind::BoundMember:
      case clang::BuiltinType::Kind::PseudoObject:
      case clang::BuiltinType::Kind::UnknownAny:
      case clang::BuiltinType::Kind::ARCUnbridgedCast:
      case clang::BuiltinType::Kind::BuiltinFn:
      case clang::BuiltinType::Kind::OMPArraySection: {
         entity.dump();
         fatal("unhandled builtin type: ",
               entity.getNameAsCString(clang::PrintingPolicy(clang::LangOptions())));
      }
      case clang::BuiltinType::Kind::SChar: // 'signed char', explicitly qualified
         return ctx.builtins._schar;
      case clang::BuiltinType::Kind::Short:
         return ctx.builtins._short;
      case clang::BuiltinType::Kind::Int:
         return ctx.builtins._int;
      case clang::BuiltinType::Kind::Long:
         return ctx.builtins._long;
      case clang::BuiltinType::Kind::LongLong:
         return ctx.builtins._long_long;
      case clang::BuiltinType::Kind::UChar: // 'unsigned char', explicitly qualified
         return ctx.builtins._char;
      case clang::BuiltinType::Kind::UShort:
         return ctx.builtins._ushort;
      case clang::BuiltinType::Kind::UInt:
         return ctx.builtins._uint;
      case clang::BuiltinType::Kind::ULong:
         return ctx.builtins._ulong;
      case clang::BuiltinType::Kind::ULongLong:
         return ctx.builtins._ulong_long;
      case clang::BuiltinType::Kind::Char_U: // 'char' for targets where it's unsigned
         return ctx.builtins._char;
      case clang::BuiltinType::Kind::WChar_U: // 'wchar_t' for targets where it's unsigned
      case clang::BuiltinType::Kind::WChar_S: // 'wchar_t' for targets where it's signed
                                              // FIXME: do we care about the difference?
         return ctx.builtins._wchar;
      case clang::BuiltinType::Kind::Char8: // 'char8_t' in C++20
         return ctx.builtins._char8;
      case clang::BuiltinType::Kind::Char16: // 'char16_t' in C++
         return ctx.builtins._char16;
      case clang::BuiltinType::Kind::Char32: // 'char32_t' in C++
         return ctx.builtins._char32;
      case clang::BuiltinType::Kind::Bool: // 'bool' in C++, '_Bool' in C99
         return ctx.builtins._bool;
      case clang::BuiltinType::Kind::Float:
         return ctx.builtins._float;
      case clang::BuiltinType::Kind::Double:
         return ctx.builtins._double;
      case clang::BuiltinType::Kind::LongDouble:
         return ctx.builtins._long_double;
      case clang::BuiltinType::Kind::Void: // 'void'
         return ctx.builtins._void;
      case clang::BuiltinType::Kind::Int128: // '__int128_t'
         return ctx.builtins._int128;
      case clang::BuiltinType::Kind::UInt128: // '__uint128_t'
         return ctx.builtins._uint128;
      case clang::BuiltinType::Kind::Half: // 'half' in OpenCL, '__fp16' in ARM NEON.
         // FIXME: return ctx.builtins._ocl_float16;
         return ctx.builtins._neon_float16;
      case clang::BuiltinType::Kind::Float16: // '_Float16'
         return ctx.builtins._float16;
      case clang::BuiltinType::Kind::Float128: // '__float128'
         return ctx.builtins._float128;
      case clang::BuiltinType::Kind::NullPtr: // type of 'nullptr'
         return ctx.builtins._nullptr;
      /* FIXME:
         pub TypeNode _size_t = Builtins::init("cpp_size_t");
         pub TypeNode _ptrdiff_t = Builtins::init("cpp_ptrdiff_t");
         pub TypeNode _max_align_t = Builtins::init("cpp_max_align_t");
         pub TypeNode _byte = Builtins::init("cpp_byte");
      */
      default:
         entity.dump();
         fatal("unreachable: builtin type: ",
               entity.getNameAsCString(clang::PrintingPolicy(clang::LangOptions())));
   }
} // namespace ensnare::ct

MAP(clang::QualType) {
   // Too niche to care about for now.
   if (entity.isRestrictQualified() || entity.isVolatileQualified()) {
      fatal("volatile and restrict qualifiers unsupported");
   }
   auto type = entity.getTypePtr();
   if (type) {
      auto result = map(ctx, *type);
      if (entity.isConstQualified()) { // FIXME: `isLocalConstQualified`: do
                                       // we care about local vs non-local?
                                       // This will be usefull later.
                                       // result->flag_const();
      }
      return result;
   } else {
      fatal("QualType inner type was null");
   }
}

MAP(Node<TypeDecl>) {
   if (is<AliasTypeDecl>(entity)) {
      return node<Type>(AtomType(deref<AliasTypeDecl>(entity).name));
   } else if (is<EnumTypeDecl>(entity)) {
      return node<Type>(AtomType(deref<EnumTypeDecl>(entity).name));
   } else if (is<RecordTypeDecl>(entity)) {
      return node<Type>(AtomType(deref<RecordTypeDecl>(entity).name));
   } else {
      fatal("unreachable: TypeDecl");
   }
}

MAP(const clang::ElaboratedType&) { return map(ctx, entity.getNamedType()); }

MAP(const clang::NamedDecl&) {
   auto type = ctx.lookup(entity);
   if (type) {
      return *type;
   } else {
      wrap(ctx, entity);
      auto type = ctx.lookup(entity);
      if (type) {
         return *type;
      } else {
         entity.dump();
         fatal("failed to map type");
      }
   }
}

// FIXME: add the restrict_wrap counter.

// If a `Type` has a `Decl` representation, always map that. It seems to work.
MAP(const clang::TypedefType&) { return map(ctx, entity.getDecl()); }

MAP(const clang::RecordType&) { return map(ctx, entity.getDecl()); }

MAP(const clang::EnumType&) { return map(ctx, entity.getDecl()); }

MAP(const clang::PointerType&) { return node<Type>(PtrType(map(ctx, entity.getPointeeType()))); }

MAP(const clang::ReferenceType&) { return node<Type>(RefType(map(ctx, entity.getPointeeType()))); }

MAP(const clang::VectorType&) {
   // The hope is that this is an aliased type as part of `typedef` / `using`
   // and the internals don't matter.
   return node<Type>(OpaqueType());
   // switch (entity->getVectorKind())
   // case clang::VectorType::VectorKind::GenericVector:
   // case clang::VectorType::VectorKind::AltiVecVector:
   // case clang::VectorType::VectorKind::AltiVecPixel:
   // case clang::VectorType::VectorKind::AltiVecBool:
   // case clang::VectorType::VectorKind::NeonVector:
   // case clang::VectorType::VectorKind::NeonPolyVector: {
   //    entity->dump();
   //    fatal("unhandled vector type");
   // }
}

MAP(const clang::ParenType&) { return map(ctx, entity.getInnerType()); }

MAP(const clang::DecltypeType&) { return map(ctx, entity.getUnderlyingType()); }

// return a suitable name from a named declaration.
fn nim_name(Context& ctx, const clang::NamedDecl& decl) -> Str {
   auto name = decl.getDeclName();
   switch (name.getNameKind()) {
      case clang::DeclarationName::NameKind::Identifier: {
         return decl.getNameAsString();
      }
      case clang::DeclarationName::NameKind::ObjCZeroArgSelector:
      case clang::DeclarationName::NameKind::ObjCOneArgSelector:
      case clang::DeclarationName::NameKind::ObjCMultiArgSelector: {
         fatal("obj-c not supported");
      }
      case clang::DeclarationName::NameKind::CXXConstructorName:
      case clang::DeclarationName::NameKind::CXXDestructorName:
      case clang::DeclarationName::NameKind::CXXConversionFunctionName:
      case clang::DeclarationName::NameKind::CXXOperatorName:
      case clang::DeclarationName::NameKind::CXXDeductionGuideName:
      case clang::DeclarationName::NameKind::CXXLiteralOperatorName:
      case clang::DeclarationName::NameKind::CXXUsingDirective:
      default:
         decl.dump();
         fatal("unhandled decl name");
   }
}

fn qualified_nim_name(Context& ctx, const clang::NamedDecl& decl) -> Str {
   /*
   FIXME: steal this without the bad parts.

   const DeclContext* Ctx = getDeclContext();

   // For ObjC methods and properties, look through categories and use the
   // interface as context.
   if (auto* MD = dyn_cast<ObjCMethodDecl>(this)) {
      if (auto* ID = MD->getClassInterface()) {
         Ctx = ID;
      }
   }
   if (auto* PD = dyn_cast<ObjCPropertyDecl>(this)) {
      if (auto* MD = PD->getGetterMethodDecl()) {
         if (auto* ID = MD->getClassInterface()) {
            Ctx = ID;
         }
      }
   }

   if (Ctx->isFunctionOrMethod())
   {return;}

   using ContextsTy = SmallVector<const DeclContext*, 8>;
   ContextsTy Contexts;

   // Collect named contexts.
   while (Ctx) {
      if (isa<NamedDecl>(Ctx)) {
         Contexts.push_back(Ctx); }
      Ctx = Ctx->getParent();
   }

   for (const DeclContext* DC : llvm::reverse(Contexts)) {
      if (const auto* Spec = dyn_cast<ClassTemplateSpecializationDecl>(DC)) {
         OS << Spec->getName();
         const TemplateArgumentList& TemplateArgs = Spec->getTemplateArgs();
         printTemplateArgumentList(OS, TemplateArgs.asArray(), P);
      } else if (const auto* ND = dyn_cast<NamespaceDecl>(DC)) {
         if (P.SuppressUnwrittenScope && (ND->isAnonymousNamespace() || ND->isInline())) {
            continue;
         }
         if (ND->isAnonymousNamespace()) {
            OS << (P.MSVCFormatting ? "`anonymous namespace\'" : "(anonymous namespace)");
         } else {
            OS << *ND;
         }
      } else if (const auto* RD = dyn_cast<RecordDecl>(DC)) {
         if (!RD->getIdentifier()) {
            OS << "(anonymous " << RD->getKindName() << ')';
         } else {
            OS << *RD;
         }
      } else if (const auto* FD = dyn_cast<FunctionDecl>(DC)) {
         const FunctionProtoType* FT = nullptr;
         if (FD->hasWrittenPrototype()) {
            FT = dyn_cast<FunctionProtoType>(FD->getType()->castAs<FunctionType>());
         }

         OS << *FD << '(';
         if (FT) {
            unsigned NumParams = FD->getNumParams();
            for (unsigned i = 0; i < NumParams; ++i) {
               if (i) {
                  OS << ", ";
               }
               OS << FD->getParamDecl(i)->getType().stream(P);
            }

            if (FT->isVariadic()) {
               if (NumParams > 0) {
                  OS << ", ";
               }
               OS << "...";
            }
         }
         OS << ')';
      } else if (const auto* ED = dyn_cast<EnumDecl>(DC)) {
         // C++ [dcl.enum]p10: Each enum-name and each unscoped
         // enumerator is declared in the scope that immediately contains
         // the enum-specifier. Each scoped enumerator is declared in the
         // scope of the enumeration.
         // For the case of unscoped enumerator, do not include in the qualified
         // name any information about its enum enclosing scope, as its visibility
         // is global.
         if (ED->isScoped())
            OS << *ED;
         else
            continue;
      } else {
         OS << *cast<NamedDecl>(DC);
      }
      OS << "::";
   }
   */
   // auto name = decl.getDeclName();
   // switch (name.getNameKind()) {
   // case clang::DeclarationName::NameKind::Identifier: {
   //   return decl.getNameAsString();
   //}
   // case clang::DeclarationName::NameKind::ObjCZeroArgSelector:
   // case clang::DeclarationName::NameKind::ObjCOneArgSelector:
   // case clang::DeclarationName::NameKind::ObjCMultiArgSelector: {
   //   fatal("obj-c not supported");
   //}
   // case clang::DeclarationName::NameKind::CXXConstructorName:
   // case clang::DeclarationName::NameKind::CXXDestructorName:
   // case clang::DeclarationName::NameKind::CXXConversionFunctionName:
   // case clang::DeclarationName::NameKind::CXXOperatorName:
   // case clang::DeclarationName::NameKind::CXXDeductionGuideName:
   // case clang::DeclarationName::NameKind::CXXLiteralOperatorName:
   // case clang::DeclarationName::NameKind::CXXUsingDirective:
   // default:
   //   decl.dump();
   //   fatal("unhandled decl name");
   //}
   return replace(decl.getQualifiedNameAsString(), "::", "-");
}

// Retrieve all the information we have about a declaration.
template <typename T> fn get_definition(const T& decl) -> const T& {
   auto result = decl.getDefinition();
   if (result) {
      return *result;
   } else {
      return decl;
   }
}

WRAP(Record) {
   RecordTypeDecl record(qualified_nim_name(ctx, decl), decl.getQualifiedNameAsString(),
                         expect(ctx.header(decl), "failed to canonicalize source location"));
   auto type_decl = node<TypeDecl>(record);
   ctx.set(decl, map(ctx, type_decl));
   ctx.add(type_decl);
}

fn get_cstddef_item(Context& ctx, const clang::NamedDecl& decl) -> Opt<Node<Type>> {
   auto qualified_name = decl.getQualifiedNameAsString();
   if (qualified_name == "std::size_t" || qualified_name == "size_t") {
      return ctx.builtins._size;
   } else if (qualified_name == "std::ptrdiff_t" || qualified_name == "ptrdiff_t") {
      return ctx.builtins._ptrdiff;
   } else if (qualified_name == "std::nullptr_t") {
      return ctx.builtins._nullptr;
   } else {
      return {};
   }
}

WRAP(TypedefName) {
   // This could be c++ `TypeAliasDecl` or a `TypedefDecl`.
   // Either way, we produce `type AliasName* = UnderlyingType`

   if (decl.getKind() == clang::Decl::Kind::ObjCTypeParam) {
      fatal("obj-c is unsupported");
   } else {
      // header_check(ctx.filename(decl.getLocation))
      // if (!ctx.file_filter(decl)) {
      //   return;
      //}
      auto cstddef_item = get_cstddef_item(ctx, decl);
      if (cstddef_item) {
         ctx.set(decl, *cstddef_item);
      } else {
         auto qualified_name = decl.getQualifiedNameAsString();
         AliasTypeDecl alias(qualified_nim_name(ctx, decl), map(ctx, decl.getUnderlyingType()));
         auto type_decl = node<TypeDecl>(alias);
         ctx.set(decl, map(ctx, type_decl));
         ctx.add(type_decl);
      }
   }
}

fn wrap_formal(Context& ctx, const clang::ParmVarDecl& param) -> ParamDecl {
   return ParamDecl(param.getNameAsString(), map(ctx, param.getType()));
}

fn wrap_formals(Context& ctx, const clang::FunctionDecl& decl) -> Vec<ParamDecl> {
   Vec<ParamDecl> result;
   for (auto param : decl.parameters()) {
      result.push_back(wrap_formal(ctx, *param));
   }
   return result;
}

fn map_return_type(Context& ctx, const clang::FunctionDecl& decl) -> Opt<Node<Type>> {
   auto result = decl.getReturnType();
   if (result->isVoidType()) {
      return {};
   } else {
      return map(ctx, result);
   }
}

// Wrap a free non templated function.
// It could be in a namespace.
fn wrap_non_template(Context& ctx, const clang::FunctionDecl& decl) {
   ctx.add(node<RoutineDecl>(
       FunctionDecl(decl.getNameAsString(), decl.getQualifiedNameAsString(),
                    expect(ctx.header(decl), "failed to canonicalize source location"),
                    wrap_formals(ctx, decl), map_return_type(ctx, decl))));
}

fn is_visible(const clang::VarDecl& decl) -> bool {
   return decl.hasGlobalStorage() && !decl.isStaticLocal();
}

WRAP(Var) {
   if (ctx.access_filter(decl) && is_visible(decl)) {
      ctx.add(node<VariableDecl>(qualified_nim_name(ctx, decl), decl.getQualifiedNameAsString(),
                                 expect(ctx.header(decl), "failed to canonicalize source location"),
                                 map(ctx, decl.getType())));
   }
}

WRAP(Function) {
   switch (decl.getTemplatedKind()) {
      case clang::FunctionDecl::TK_NonTemplate:
         wrap_non_template(ctx, decl);
         break;
      case clang::FunctionDecl::TK_FunctionTemplate:
      case clang::FunctionDecl::TK_MemberSpecialization:
      case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization: {
         fatal("unhandled template function variety");
      }
   }
}

fn wrap_non_template(Context& ctx, const clang::CXXConstructorDecl& decl) {
   if (ctx.access_filter(decl)) {
      ctx.add(node<RoutineDecl>(
          ConstructorDecl(decl.getQualifiedNameAsString(),
                          expect(ctx.header(decl), "failed to canonicalize source location"),
                          map(ctx, decl.getParent()), wrap_formals(ctx, decl))));
   }
}

WRAP(CXXConstructor) {
   switch (decl.getTemplatedKind()) {
      case clang::FunctionDecl::TK_NonTemplate:
         wrap_non_template(ctx, decl);
         break;
      case clang::FunctionDecl::TK_FunctionTemplate:
      case clang::FunctionDecl::TK_MemberSpecialization:
      case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization: {
         fatal("unhandled template constructor variety");
      }
   }
}

fn wrap_non_template(Context& ctx, const clang::CXXMethodDecl& decl) {
   if (ctx.access_filter(decl)) {
      ctx.add(node<RoutineDecl>(MethodDecl(
          decl.getNameAsString(), decl.getQualifiedNameAsString(),
          expect(ctx.header(decl), "failed to canonicalize source location"),
          map(ctx, decl.getParent()), wrap_formals(ctx, decl), map_return_type(ctx, decl))));
   }
}

WRAP(CXXMethod) {
   switch (decl.getTemplatedKind()) {
      case clang::FunctionDecl::TK_NonTemplate:
         wrap_non_template(ctx, decl);
         break;
      case clang::FunctionDecl::TK_FunctionTemplate:
      case clang::FunctionDecl::TK_MemberSpecialization:
      case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization: {
         fatal("unhandled template constructor variety");
      }
   }
}

fn map(Context& ctx, clang::EnumDecl::enumerator_range fields) -> Vec<EnumFieldDecl> {
   Vec<EnumFieldDecl> result;
   for (auto field : fields) {
      // FIXME: expose if a value was explicitly provided.
      // auto expr = field->getInitExpr();
      // if (expr) {
      // } else {
      // }
      result.push_back(EnumFieldDecl(field->getNameAsString(), field->getInitVal().getExtValue()));
   }
   return result;
}

WRAP(Enum) {
   if (ctx.access_filter(decl)) {
      ctx.add(node<TypeDecl>(
          EnumTypeDecl(qualified_nim_name(ctx, decl), decl.getQualifiedNameAsString(),
                       expect(ctx.header(decl), "failed to canonicalize source location"),
                       map(ctx, decl.enumerators()))));
   }
}

#define DISPATCH(kind)                                                                             \
   case clang::Type::TypeClass::kind: {                                                            \
      return map(ctx, llvm::cast<clang::kind##Type>(entity));                                      \
   }

MAP(const clang::Type&) {
   switch (entity.getTypeClass()) {
      DISPATCH(Builtin);
      DISPATCH(Elaborated);
      DISPATCH(Record);
      DISPATCH(Enum);
      DISPATCH(Pointer);
      DISPATCH(LValueReference);
      DISPATCH(RValueReference);
      DISPATCH(Typedef);
      DISPATCH(Vector);
      DISPATCH(Paren);
      DISPATCH(Decltype);
      default:
         entity.dump();
         fatal("unhandled mapping: ", entity.getTypeClassName());
   }
}

#undef DISPATCH

// This will call `wrap(clang::kind##Decl)` within the dispatch case.
#define DISPATCH(kind)                                                                             \
   case clang::Decl::Kind::kind: {                                                                 \
      wrap(ctx, llvm::cast<clang::kind##Decl>(decl));                                              \
      break;                                                                                       \
   }

// This will dispatch `wrap(clang::kind##Decl)` for any of `clang::kind##Decl`'s children.
#define DISPATCH_ANY(kind)                                                                         \
   case clang::Decl::Kind::first##kind... clang::Decl::Kind::last##kind: {                         \
      wrap(ctx, llvm::cast<clang::kind##Decl>(decl));                                              \
      break;                                                                                       \
   }

// silently discard these `Decl`s and continue.
#define DISCARD(kind)                                                                              \
   case clang::Decl::Kind::kind: {                                                                 \
      break;                                                                                       \
   }

// _switch<Typedef>(decl, );

WRAP(Named) {
   auto header = ctx.header(decl);
   if (header) {
      switch (decl.getKind()) {
         DISCARD(NamespaceAlias);
         DISCARD(Namespace);
         DISCARD(Field);
         DISCARD(ParmVar);
         DISCARD(Using);
         DISCARD(EnumConstant);

         DISPATCH_ANY(TypedefName);
         DISPATCH_ANY(Record);
         DISPATCH(Enum);
         DISPATCH(Function);
         DISPATCH(CXXMethod);
         DISPATCH(CXXConstructor);
         DISPATCH(Var);
         default:
            decl.dump();
            fatal("unhandled wrapping: ", decl.getDeclKindName(), "Decl");
      }
   }
}

fn wrap(Context& ctx, const clang::Decl& decl) -> void {
   switch (decl.getKind()) {
      // pre NamedDecls
      DISCARD(AccessSpec);
      DISCARD(Block);
      DISCARD(Captured);
      DISCARD(ClassScopeFunctionSpecialization);
      DISCARD(Empty);
      DISCARD(Export);
      DISCARD(ExternCContext);
      DISCARD(FileScopeAsm);
      DISCARD(Friend);
      DISCARD(FriendTemplate);
      DISCARD(Import);
      DISCARD(LifetimeExtendedTemporary);
      DISCARD(LinkageSpec);
      // after NamedDecls
      DISCARD(ObjCPropertyImpl);
      DISCARD(OMPAllocate);
      DISCARD(OMPRequires);
      DISCARD(OMPThreadPrivate);
      DISCARD(PragmaComment);
      DISCARD(PragmaDetectMismatch);
      DISCARD(RequiresExprBody);
      DISCARD(StaticAssert);
      DISCARD(TranslationUnit);
      // NamedDecls
      DISPATCH_ANY(Named)
      default:
         decl.dump();
         fatal("unhandled wrapping: ", decl.getDeclKindName(), "Decl");
   }
}

#undef WRAP
#undef NO_WRAP
#undef MAP

#undef DISPATCH
#undef DISPATCH_ANY
#undef DISCARD

// Instantiate this class to do the work. The Config will grow in complexity to support
// introspection to produce wrappers that make c++ devs jealous.
class BindAction : public clang::RecursiveASTVisitor<BindAction> {
   priv const Config& cfg;
   priv const std::unique_ptr<clang::ASTUnit> tu;
   priv Context ctx;

   // Load a translation unit from user provided arguments with additional include path
   // arguments.
   priv fn parse_tu() -> std::unique_ptr<clang::ASTUnit> {
      Vec<Str> args = {"-xc++"};
      auto includes = include_paths();
      args.insert(args.end(), includes.begin(), includes.end());
      args.insert(args.end(), cfg.clang_args().begin(), cfg.clang_args().end());
      return clang::tooling::buildASTFromCodeWithArgs(cfg.header_file(), args, "ensnare_headers.h",
                                                      "ensnare");
   }

   // Render and write out our decls.
   priv fn finalize() {
      Str output = "import ensnare/runtime\n";
      if (ctx.type_decls().size() != 0) {
         output += "\n# --- types\n\n";
         output += "type\n";
         for (const auto& type_decl : ctx.type_decls()) {
            output += rendering::indent(rendering::render(type_decl));
         }
      }
      if (ctx.routine_decls().size() != 0) {
         output += "\n# --- routines\n\n";
         for (const auto& routine_decl : ctx.routine_decls()) {
            output += rendering::render(routine_decl);
         }
      }
      if (ctx.variable_decls().size() != 0) {
         output += "\n# --- variables\n\n";
         output += "var\n";
         for (const auto& variable_decl : ctx.variable_decls()) {
            output += rendering::indent(rendering::render(variable_decl));
         }
      }
      os::write_file(os::set_file_ext(ctx.cfg.output(), ".nim"), output);
   }

   pub BindAction(const Config& cfg) : cfg(cfg), tu(parse_tu()), ctx(cfg, tu->getASTContext()) {
      // Collect all our declarations.
      auto result = TraverseAST(tu->getASTContext());
      if (!result) { // Idk wtf this thing means? It is true even with errors...
         fatal("BindAction failed");
      } else {
         finalize();
      }
   }

   pub fn VisitDecl(clang::Decl* decl) -> bool {
      if (decl) {
         wrap(ctx, *decl);
      } else {
         fatal("unreachable: got a null visitor decl");
      }
      return true;
   }
};

fn run(int argc, const char* argv[]) { BindAction _(Config(argc, argv)); }
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

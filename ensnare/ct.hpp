#pragma once

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#ifdef __ARM_ARCH_ISA_A64
#include <experimental/filesystem>
#elif
#include <filesystem>
#endif

#include "rt.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// preserve order
#include "private/syn.hpp"

namespace ensnare::ct {
template <typename T> using Vec = std::vector<T>;

template <typename T> using Opt = std::optional<T>;

template <typename K, typename V> using Map = std::unordered_map<K, V>;

using Str = std::string;

template <typename T> using Node = std::shared_ptr<T>;

template <typename... Variants> using Union = const std::variant<Variants...>;

// Check if a `Union` is a thing. If it is you can `deref<T>(self)` it.
template <typename T, typename... Variants> fn is(const Node<Union<Variants...>>& self) -> bool {
   return std::holds_alternative<T, Variants...>(*self);
}

template <typename T, typename... Variants>
fn deref(const Node<Union<Variants...>>& self) -> const T& {
   return std::get<T, Variants...>(*self);
}

// Create a value ref counted node of type `Y` constructed from `X`.
template <typename Y, typename... Args> fn node(Args... args) -> Node<Y> {
   return std::make_shared<Y>(args...);
}

// Template recursion base case.
template <typename... Args> fn write(std::ostream* out, Args... args) {}

// Write some arguments to a output stream.
template <typename Arg, typename... Args> fn write(std::ostream* out, Arg arg, Args... args) {
   *out << arg;
   write(out, args...);
}

// Write some arguments to an ouput stream with a newline.
template <typename... Args> fn print(std::ostream* out, Args... args) {
   write(out, args...);
   write(out, '\n');
}

// Write some arguments to an `std::cout` with a newline.
template <typename... Args> fn print(Args... args) { print(&std::cout, args...); }

// Write some arguments to `std::cout` exit with error code 1.
template <typename... Args> fn fatal [[noreturn]] (Args... args) {
   print("fatal-error: ", args...);
   std::exit(1);
}

// split by '\n' for line processing.
fn split_newlines(const Str& str) -> Vec<Str> {
   Vec<Str> result;
   auto last = 0;
   for (auto i = 0; i < str.size(); i++) {
      if (str[i] == '\n') {
         Str str_result;
         for (auto j = last; j < i; j++) {
            str_result.push_back(str[j]);
         }
         result.push_back(str_result);
         last = i + 1;
      }
   }
   std::string str_result;
   for (auto j = last; j < str.size(); j++) {
      str_result.push_back(str[j]);
   }
   return result;
}

namespace os {

#ifdef __ARM_ARCH_ISA_A64
namespace fs = std::experimental::filesystem;
#elif
namespace fs = std::filesystem;
#endif

// Read the contents of `path` into memory.
fn read_file(const Str& path) -> Str {
   std::ifstream stream(fs::path{path});
   return Str(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

// Write `contents` to disk at `path`.
fn write_file(const Str& path, const Str& contents) -> void {
   auto fs_path = fs::path(path);
   if (fs_path.parent_path() != "") {
      fs::create_directories(fs_path.parent_path());
   }
   std::ofstream stream(fs_path);
   stream << contents;
   stream.close();
}

// Get a suitable location
fn temp_file(const Str& name) -> Str { return fs::temp_directory_path() / name; }

// `popen` a process and return its exit code and output if process opening and closing succeeded.
// This is probably not cross platform.
fn process_result(const Str& cmd, std::size_t buf_size = 8192) -> Opt<std::tuple<int, Str>> {
   auto file = popen(cmd.c_str(), "r");
   if (file == nullptr) {
      return {};
   }
   Str output;
   Str buf(buf_size, '\0');
   while (fgets(buf.data(), buf.size() - 1, file)) {
      for (auto c : buf) {
         if (c == '\0') {
            break;
         } else {
            output.push_back(c);
         }
      }
      for (auto& c : buf) {
         c = '\0';
      }
   }
   auto code = pclose(file);
   if (code == -1) {
      return {};
   }
   return std::tuple(code, output);
}

// Get process output or exit the application if the process failed in any way.
fn successful_process_output(const Str& cmd, std::size_t buf_size = 8192) -> Str {
   auto process_result = os::process_result(cmd);
   if (!process_result) {
      fatal("failed to execute command: ", cmd);
   }
   auto [code, output] = *process_result;
   if (code == 0) {
      return output;
   } else {
      fatal("command exited with non-zero exit code: ", cmd);
   }
}

fn is_system_header(const Str& header) -> bool {
   return (header.size() > 2 && header[0] == '<' and header[header.size() - 1] == '>');
}

fn is_cpp_file(const Str& file) -> bool {
   if (is_system_header(file)) {
      return is_cpp_file(file.substr(1, file.size() - 2));
   } else {
      fs::path path(file);
      for (const auto& ext : {".hpp", ".cpp", ".h", ".c"}) {
         if (path.extension() == ext) {
            return true;
         }
      }
      return false;
   }
}
} // namespace os

namespace cl {
namespace cl = llvm::cl;
cl::opt<bool> disable_includes("disable-includes",
                               cl::desc("do not gather system includes. FIXME: not implimented"));
cl::opt<Str> output(cl::Positional, cl::desc("output wrapper name/path"));
cl::list<Str> args(cl::ConsumeAfter, cl::desc("clang args..."));
} // namespace cl

class Config {
   // NO_COPY(Config);

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

// This is reference counted string class with a history. It should only be used like `Node<Sym`>.
class Sym {
   priv Vec<Str> _detail;
   pub Sym(const Str& name) { _detail.push_back(name); }
   pub fn latest() -> Str { return _detail.back(); }
};

class AtomType;
class PtrType;
class RefType;
class OpaqueType;

using Type = Union<AtomType, PtrType, RefType, OpaqueType>;

// An identifier.
class AtomType {
   READ_ONLY(Node<Sym>, name);
   pub AtomType(const Str& name) : _name(node<Sym>(name)) {}
   pub AtomType(const Node<Sym>& name) : _name(name) {}
};

// Just a raw pointer.
class PtrType {
   READ_ONLY(Node<Type>, pointee);
   pub PtrType(Node<Type> pointee) : _pointee(pointee) {}
};

// A c++ reference pointer.
class RefType {
   READ_ONLY(Node<Type>, pointee);
   pub RefType(Node<Type> pointee) : _pointee(pointee) {}
};

class OpaqueType {};

// A `type Foo = Bar[X, Y]` like declaration.
// These come from c++ declarations like `using Foo = Bar[X, Y];` or `typedef Bar[X, Y] Foo;`
class AliasTypeDecl {
   READ_ONLY(Node<Sym>, name);
   READ_ONLY(Node<Type>, type);
   pub AliasTypeDecl(const Str name, const Node<Type> type) : _name(node<Sym>(name)), _type(type) {}
};

class EnumFieldDecl {
   READ_ONLY(Node<Sym>, name);
   pub EnumFieldDecl(const Str name) : _name(node<Sym>(name)) {}
};

// A c++ enum of some kind. It may not be mapped as a nim enum.
class EnumTypeDecl {
   READ_ONLY(Node<Sym>, name);
   READ_ONLY(Str, cpp_name);
   READ_ONLY(Vec<EnumFieldDecl>, fields);
   pub EnumTypeDecl(const Str name, const Str cpp_name, const Vec<EnumFieldDecl> fields)
      : _name(node<Sym>(name)), _cpp_name(cpp_name), _fields(fields) {}
};

// A `struct` / `class` / `union` type declaration.
class RecordTypeDecl {
   READ_ONLY(Node<Sym>, name);
   READ_ONLY(Str, cpp_name);
   pub RecordTypeDecl(const Str name, const Str cpp_name)
      : _name(node<Sym>(name)), _cpp_name(cpp_name) {}
};

using TypeDecl = Union<AliasTypeDecl, EnumTypeDecl, RecordTypeDecl>;

class ParamDecl {
   READ_ONLY(Node<Sym>, name);
   READ_ONLY(Node<Type>, type);
   // READ_ONLY(Opt<Node<Expr>>, expr);
   pub ParamDecl(const Str name, const Node<Type> type) : _name(node<Sym>(name)), _type(type) {}
};

// A non method function declaration.
class FunctionDecl {
   READ_ONLY(Node<Sym>, name);
   READ_ONLY(Str, cpp_name);
   READ_ONLY(Vec<ParamDecl>, formals);
   READ_ONLY(Opt<Node<Type>>, return_type);
   pub FunctionDecl(const Str name, const Str cpp_name, const Vec<ParamDecl> formals,
                    const Opt<Node<Type>> return_type)
      : _name(node<Sym>(name)), _cpp_name(cpp_name), _formals(formals), _return_type(return_type) {}
};

// A c++ class/struct/union constructor
class ConstructorDecl {
   READ_ONLY(Str, cpp_name);
   READ_ONLY(Node<Type>, self);
   READ_ONLY(Vec<ParamDecl>, formals);
   pub ConstructorDecl(const Str cpp_name, const Node<Type> self, const Vec<ParamDecl> formals)
      : _cpp_name(cpp_name), _self(self), _formals(formals) {}
};

// A c++ class/struct/union method
class MethodDecl {
   READ_ONLY(Node<Sym>, name);
   READ_ONLY(Str, cpp_name);
   READ_ONLY(Node<Type>, self);
   READ_ONLY(Vec<ParamDecl>, formals);
   READ_ONLY(Opt<Node<Type>>, return_type);
   pub MethodDecl(const Str name, const Str cpp_name, const Node<Type> self,
                  const Vec<ParamDecl> formals, const Opt<Node<Type>> return_type)
      : _name(node<Sym>(name)),
        _cpp_name(cpp_name),
        _self(self),
        _formals(formals),
        _return_type(return_type) {}
};

using RoutineDecl = Union<FunctionDecl, ConstructorDecl, MethodDecl>;

class VariableDecl {
   READ_ONLY(Node<Sym>, name);
   READ_ONLY(Str, cpp_name);
   READ_ONLY(Node<Type>, type);
   pub VariableDecl(const Str& name, const Str& cpp_name, const Node<Type>& type)
      : _name(node<Sym>(name)), _cpp_name(name), _type(type) {}
};

// The cananonical types defined in rt.nim
class Builtins {
   priv static fn init(const char* name) -> Node<Type> { return node<Type>(AtomType(name)); }

   pub const Node<Type> _schar = Builtins::init("cpp_schar");
   pub const Node<Type> _short = Builtins::init("cpp_short");
   pub const Node<Type> _int = Builtins::init("cpp_int");
   pub const Node<Type> _long = Builtins::init("cpp_long");
   pub const Node<Type> _long_long = Builtins::init("cpp_long_long");
   pub const Node<Type> _uchar = Builtins::init("cpp_uchar");
   pub const Node<Type> _ushort = Builtins::init("cpp_ushort");
   pub const Node<Type> _uint = Builtins::init("cpp_uint");
   pub const Node<Type> _ulong = Builtins::init("cpp_ulong");
   pub const Node<Type> _ulong_long = Builtins::init("cpp_ulong_long");
   pub const Node<Type> _char = Builtins::init("cpp_char");
   pub const Node<Type> _wchar_t = Builtins::init("cpp_wchar_t");
   pub const Node<Type> _char8_t = Builtins::init("cpp_char8_t");
   pub const Node<Type> _char16_t = Builtins::init("cpp_char16_t");
   pub const Node<Type> _char32_t = Builtins::init("cpp_char32_t");
   pub const Node<Type> _bool = Builtins::init("cpp_bool");
   pub const Node<Type> _float = Builtins::init("cpp_float");
   pub const Node<Type> _double = Builtins::init("cpp_double");
   pub const Node<Type> _long_double = Builtins::init("cpp_long_double");
   pub const Node<Type> _void = Builtins::init("cpp_void");
   pub const Node<Type> _int128 = Builtins::init("cpp_int128");
   pub const Node<Type> _uint128 = Builtins::init("cpp_uint128");
   pub const Node<Type> _neon_float16 = Builtins::init("cpp_neon_float16");
   pub const Node<Type> _ocl_float16 = Builtins::init("cpp_ocl_float16");
   pub const Node<Type> _float16 = Builtins::init("cpp_float16");
   pub const Node<Type> _float128 = Builtins::init("cpp_float128");
   pub const Node<Type> _size_t = Builtins::init("cpp_size_t");
   pub const Node<Type> _ptrdiff_t = Builtins::init("cpp_ptrdiff_t");
   pub const Node<Type> _max_align_t = Builtins::init("cpp_max_align_t");
   pub const Node<Type> _byte = Builtins::init("cpp_byte");
   pub const Node<Type> _nullptr_t = Builtins::init("cpp_nullptr_t");

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
   pub const Builtins builtins; // Atomic types defined in `rt.nim`
   pub Context(const Config& cfg, const clang::ASTContext& ast_ctx) : cfg(cfg), ast_ctx(ast_ctx) {}

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

   pub fn access_filter(const clang::Decl& decl) const -> bool {
      // AS_protected
      // AS_private
      // AS_none
      return decl.getAccess() == clang::AS_public;
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
      return ctx.builtins._wchar_t;
   case clang::BuiltinType::Kind::Char8: // 'char8_t' in C++20
      return ctx.builtins._char8_t;
   case clang::BuiltinType::Kind::Char16: // 'char16_t' in C++
      return ctx.builtins._char16_t;
   case clang::BuiltinType::Kind::Char32: // 'char32_t' in C++
      return ctx.builtins._char32_t;
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
      return ctx.builtins._nullptr_t;
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
      return node<Type>(AtomType(deref<AliasTypeDecl>(entity).name()));
   } else if (is<EnumTypeDecl>(entity)) {
      return node<Type>(AtomType(deref<EnumTypeDecl>(entity).name()));
   } else if (is<RecordTypeDecl>(entity)) {
      return node<Type>(AtomType(deref<RecordTypeDecl>(entity).name()));
   } else {
      fatal("unreachable: TypeDecl");
   }
}

MAP(const clang::ElaboratedType&) { return map(ctx, entity.getNamedType()); }

// Here we could be looking up an aliased type, a record field, a parameter.
// We need a declaration if we want to expose this as a type.
// Currently as follows:
// 1. Check `Context::type_map` via `lookup` for a cached type.
//    This contains Decl pointer -> Node<Type> for already bound decls.
// 2. If we did not find one we should
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
   // auto def = get_definition(decl);
   RecordTypeDecl record(nim_name(ctx, decl), decl.getQualifiedNameAsString());
   auto type_decl = node<TypeDecl>(record);
   ctx.set(decl, map(ctx, type_decl));
   ctx.add(type_decl);
}

WRAP(TypedefName) {
   if (decl.getKind() == clang::Decl::Kind::ObjCTypeParam) {
      fatal("obj-c is unsupported");
   } else {
      decl.getLocation().dump(ctx.ast_ctx.getSourceManager());
      // This could be c++ `TypeAliasDecl` or a `TypedefDecl`.
      // Either way, we produce `type AliasName* = UnderlyingType`
      AliasTypeDecl alias(decl.getNameAsString(), map(ctx, decl.getUnderlyingType()));
      auto type_decl = node<TypeDecl>(alias);
      ctx.set(decl, map(ctx, type_decl));
      ctx.add(type_decl);
   }
}

fn map_return_type(Context& ctx, const clang::FunctionDecl& decl) -> Node<Type> {
   return map(ctx, decl.getReturnType());
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

// Wrap a free non templated function.
// It could be in a namespace.
fn wrap_non_template(Context& ctx, const clang::FunctionDecl& decl) {
   ctx.add(node<RoutineDecl>(FunctionDecl(decl.getNameAsString(), decl.getQualifiedNameAsString(),
                                          wrap_formals(ctx, decl), map_return_type(ctx, decl))));
}

fn is_visible(const clang::VarDecl& decl) -> bool {
   return decl.hasGlobalStorage() && !decl.isStaticLocal();
}

WRAP(Var) {
   if (ctx.access_filter(decl) && is_visible(decl)) {
      ctx.add(node<VariableDecl>(decl.getNameAsString(), decl.getQualifiedNameAsString(),
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
   ctx.add(node<RoutineDecl>(ConstructorDecl(decl.getQualifiedNameAsString(),
                                             map(ctx, decl.getParent()), wrap_formals(ctx, decl))));
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
   ctx.add(node<RoutineDecl>(MethodDecl(decl.getNameAsString(), decl.getQualifiedNameAsString(),
                                        map(ctx, decl.getParent()), wrap_formals(ctx, decl),
                                        map_return_type(ctx, decl))));
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

// --- dispatching, presumably the hot path.

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
   default:
      entity.dump();
      fatal("unhandled mapping: ", entity.getTypeClassName());
   }
}

#undef DISPATCH

#undef WRAP
#undef NO_WRAP
#undef MAP

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

fn wrap(Context& ctx, const clang::Decl& decl) -> void {
   switch (decl.getKind()) {
      DISCARD(TranslationUnit);
      DISCARD(Namespace);
      DISCARD(Field);
      DISCARD(ParmVar);
      DISCARD(AccessSpec);
      DISPATCH_ANY(TypedefName);
      DISPATCH_ANY(Record);
      DISPATCH(Function);
      DISPATCH(CXXMethod);
      DISPATCH(CXXConstructor);
      DISPATCH(Var);
   default:
      decl.dump();
      fatal("unhandled wrapping: ", decl.getDeclKindName(), "Decl");
   }
}
// Get a suitable location to store temp file.
fn temp(Str name) -> Str { return os::temp_file("ensnare_system_includes_" + name); }

// Get some verbose compiler logs to parse.
fn get_raw_include_paths() -> Str {
   os::write_file(temp("test"), ""); // so the compiler does not error.
   Str cmd = "clang++ -xc++ -c -v " + temp("test") + " 2>&1";
   return os::successful_process_output(cmd);
}

// Parse include paths out of some verbose compiler logs.
fn include_paths() -> Vec<Str> {
   Vec<Str> result;
   auto lines = split_newlines(get_raw_include_paths());
   if (lines.size() == 0) {
      return result;
   }
   auto start = std::find(lines.begin(), lines.end(), "#include <...> search starts here:");
   auto stop = std::find(lines.begin(), lines.end(), "End of search list.");
   if (start == lines.end() || stop == lines.end()) {
      fatal("failed to parse include paths");
   }
   for (auto it = start + 1; it != stop; it++) {
      if (it->size() > 0) {
         result.push_back("-isystem" + ((*it)[0] == ' ' ? it->substr(1) : *it));
      }
   }
   return result;
}

#undef DISPATCH
#undef DISPATCH_ANY
#undef DISCARD

namespace rendering {
const std::size_t indent_size = 3;
fn indent() -> Str {
   Str result;
   for (auto i = 0; i < indent_size; i++) {
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

fn render(const Node<Sym>& sym) -> Str { return sym->latest(); }

fn render(const AtomType& typ) -> Str { return render(typ.name()); }

fn render(const PtrType& typ) -> Str { return "ptr " + render(typ.pointee()); }

fn render(const RefType& typ) -> Str { return "var " + render(typ.pointee()); }

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

fn render(const AliasTypeDecl& decl) -> Str {
   return render(decl.name()) + "* = " + render(decl.type()) + "\n";
}

fn render(const EnumFieldDecl& decl) -> Str { return indent() + render(decl.name()) + '\n'; }

fn render(const EnumTypeDecl& decl) -> Str {
   Str result =
       render(decl.name()) + "* " + render_pragmas({import_cpp(decl.cpp_name())}) + " = enum\n";
   for (const auto& field : decl.fields()) {
      result += render(field);
   }
   return result;
}

fn render(const RecordTypeDecl& decl) -> Str { return render(decl.name()) + "* = object\n"; }

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

fn render(const ParamDecl& decl) -> Str { return render(decl.name()) + ": " + render(decl.type()); }

fn render(const FunctionDecl& decl) -> Str {
   auto result = "proc " + render(decl.name()) + "*(";
   auto first = true;
   for (const auto& formal : decl.formals()) {
      if (not first) {
         result += ", ";
      }
      result += render(formal);
      first = false;
   }
   result += ")";
   if (decl.return_type()) {
      result += ": ";
      result += render(*decl.return_type());
   }
   result += "\n" + indent() + render_pragmas({import_cpp(decl.cpp_name() + "(@)")}) + "\n";
   return result;
}

fn render(const ConstructorDecl& decl) -> Str {
   Str result = "proc `{}`*(Self: type[";
   result += render(decl.self()) + "]";
   for (const auto& formal : decl.formals()) {
      result += ", " + render(formal);
   }
   result += "): " + render(decl.self());
   result += "\n" + indent() + render_pragmas({import_cpp(decl.cpp_name() + "(@)")}) + "\n";
   return result;
}

fn render(const MethodDecl& decl) -> Str {

   Str result = "proc " + render(decl.name()) + "*(self: ";
   result += render(decl.self());
   for (const auto& formal : decl.formals()) {
      result += ", " + render(formal);
   }
   result += ")";
   if (decl.return_type()) {
      result += ": ";
      result += render(*decl.return_type());
   }
   result += "\n" + indent() + render_pragmas({import_cpp(decl.cpp_name() + "(@)")}) + "\n";
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
   return render(decl->name()) + "* " + render_pragmas({import_cpp(decl->cpp_name())}) + ": " +
          render(decl->type()) + "\n";
}

} // namespace rendering

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
      Str output;
      output += "# generated bindings --- machine dependent\n";
      if (ctx.type_decls().size() != 0) {
         output += "# type section\n";
         output += "type\n";
         for (const auto& type_decl : ctx.type_decls()) {
            output += rendering::indent(rendering::render(type_decl));
         }
      }
      if (ctx.routine_decls().size() != 0) {
         output += "# function section\n";
         for (const auto& routine_decl : ctx.routine_decls()) {
            output += rendering::render(routine_decl);
         }
      }
      if (ctx.variable_decls().size() != 0) {
         output += "# variable section\n";
         output += "var\n";
         for (const auto& variable_decl : ctx.variable_decls()) {
            output += rendering::indent(rendering::render(variable_decl));
         }
      }
      os::write_file(ctx.cfg.output() + ".nim", output);
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

fn run(int argc, const char* argv[]) {
   const Config cfg(argc, argv);
   BindAction action(cfg);
}
} // namespace ensnare::ct

#include "private/undef_syn.hpp"


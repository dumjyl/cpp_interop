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
template <typename... Args> fn print(Args... args) {
   print(&std::cout, args...);
}

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
fn temp_file(const Str& name) -> Str {
   return fs::temp_directory_path() / name;
}

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
cl::opt<Str> name(cl::Positional, cl::desc("wrapper name"));
cl::list<Str> args(cl::ConsumeAfter, cl::desc("clang args..."));
} // namespace cl

class Config {
   NO_COPY(Config);

   READ_ONLY(Vec<Str>, headers);
   READ_ONLY(Str, name);
   READ_ONLY(Vec<Str>, clang_args);

   pub Config(int argc, const char* argv[]) {
      llvm::cl::ParseCommandLineOptions(argc, argv);
      this->_name = cl::name;
      for (const auto& arg : cl::args) {
         if (os::is_cpp_file(arg)) {
            this->_headers.push_back(arg);
         } else {
            this->_clang_args.push_back(arg);
         }
      }
   };

   // A header file with all the `--header` args added together.
   pub fn header_file() const -> Str {
      Str result;
      for (const auto& header : this->_headers) {
         result += "#include ";
         if (os::is_system_header(header)) {
            result += header;
         } else {
            result += "\"" + header + "\"";
         }
         result += '\n';
      }
      return result;
   }
};

// Get a suitable location to store temp file.
fn temp(Str name) -> Str {
   return os::temp_file("ensnare_system_includes_" + name);
}

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

// Load a translation unit from user provided arguments with additional include path arguments.
fn parse_translation_unit(const Config& cfg) -> std::unique_ptr<clang::ASTUnit> {
   Vec<Str> args = {"-xc++"};
   auto includes = include_paths();
   args.insert(args.end(), includes.begin(), includes.end());
   args.insert(args.end(), cfg.clang_args().begin(), cfg.clang_args().end());
   return clang::tooling::buildASTFromCodeWithArgs(cfg.header_file(), args, "ensnare_headers.h",
                                                   "ensnare");
}

// This is reference counted string class with a history. It should only be used like `Node<Sym`>.
class Sym {
   priv Vec<Str> _detail;
   pub Sym(const Str& name) {
      this->_detail.push_back(name);
   }
   pub fn latest() -> Str {
      return _detail.back();
   }
};

class AtomType;
class PtrType;
class OpaqueType;

using Type = Union<AtomType, PtrType, OpaqueType>;

// An identifier.
class AtomType {
   READ_ONLY(Node<Sym>, name);
   pub AtomType(const Str& name) : _name(node<Sym>(name)) {}
   pub AtomType(const Node<Sym>& name) : _name(name) {}
};

// Just a raw pointer.
class PtrType {
   READ_ONLY(Node<Type>, type);
   pub PtrType(Node<Type> type) : _type(type) {}
};

// A c++ reference pointer.
class RefPtrType {
   READ_ONLY(Node<Type>, type);
   pub RefPtrType(Node<Type> type) : _type(type) {}
};

// This class should only come on the right hand side of a AliasTypeDecl.
class OpaqueType {};

// A `type Foo = Bar[X, Y]` like declaration.
// These come from c++ declarations like `using Foo = Bar[X, Y];` or `typedef Bar[X, Y] Foo;`
class AliasTypeDecl {
   READ_ONLY(Node<Sym>, name);
   READ_ONLY(Node<Type>, type);
   pub AliasTypeDecl(const Str& name, Node<Type> type) : _name(node<Sym>(name)), _type(type) {}
};

class EnumFieldDecl {
   READ_ONLY(Node<Sym>, name);
   pub EnumFieldDecl(const Str& name) : _name(node<Sym>(name)) {}
};

// A c++ enum of some kind. It may not be mapped as a nim enum.
class EnumTypeDecl {
   READ_ONLY(Node<Sym>, name);
   READ_ONLY(Str, cpp_name);
   READ_ONLY(Vec<EnumFieldDecl>, fields);
   pub EnumTypeDecl(const Str& name, const Str& cpp_name, Vec<EnumFieldDecl> fields)
      : _name(node<Sym>(name)), _cpp_name(cpp_name), _fields(fields) {}
};

// A `struct` / `class` / `union` type declaration.
class RecordTypeDecl {
   READ_ONLY(Node<Sym>, name);
   READ_ONLY(Str, cpp_name);
   pub RecordTypeDecl(const Str& name, const Str& cpp_name)
      : _name(node<Sym>(name)), _cpp_name(cpp_name) {}
};

using TypeDecl = Union<AliasTypeDecl, EnumTypeDecl, RecordTypeDecl>;

// A non method function declaration.
class FreeFunctionDecl {
   READ_ONLY(Node<Sym>, name);
   pub FreeFunctionDecl(const Str& name) : _name(node<Sym>(name)) {}
};

using FunctionDecl = Union<FreeFunctionDecl>;

class VariableDecl {
   READ_ONLY(Node<Sym>, name);
   READ_ONLY(Str, cpp_name);
   READ_ONLY(Node<Type>, type);
   pub VariableDecl(const Str& name, const Str& cpp_name, const Node<Type>& type)
      : _name(node<Sym>(name)), _cpp_name(name), _type(type) {}
};

// The cananonical types defined in rt.nim
class Builtins {
   priv static fn init(const char* name) -> Node<Type> {
      return node<Type>(AtomType(name));
   }

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
   priv Map<const clang::Decl*, Node<Type>>
       type_lookup;                            // For mapping bound declarations to exisiting types.
   READ_ONLY(Vec<Node<TypeDecl>>, type_decls); // Types to output.
   READ_ONLY(Vec<Node<FunctionDecl>>, function_decls); // Functions to output.
   READ_ONLY(Vec<Node<VariableDecl>>, variable_decls); // Variables to output.

   pub Config& cfg;
   pub Builtins builtins; // Atomic types defined in `rt.nim`
   pub Context(Config& cfg) : cfg(cfg) {}

   pub fn lookup(const clang::Decl* decl) const -> Opt<Node<Type>> {
      auto decl_type = this->type_lookup.find(decl);
      if (decl_type == this->type_lookup.end()) {
         return {};
      } else {
         return decl_type->second;
      }
   }

   pub fn set(const clang::Decl* decl, Node<Type> type) {
      auto maybe_type = this->lookup(decl);
      if (maybe_type) {
         fatal("FIXME: duplicate set");
      } else {
         this->type_lookup[decl] = type;
      }
   }

   pub fn add(Node<TypeDecl> decl) {
      this->_type_decls.push_back(decl);
   }

   pub fn add(Node<FunctionDecl> decl) {
      this->_function_decls.push_back(decl);
   }

   pub fn add(Node<VariableDecl> decl) {
      this->_variable_decls.push_back(decl);
   }
};

// We have the `wrap` operation which add declarative bindings to the
//`Context` from the `Stmt`s
// we / visit. / We have the `map` operation which produces a suitable ir type
// from a `Type`,
//`QualType` or / even some kinds of `Stmt` (a decltype for example).
//

#define WRAP(T) fn wrap(Context& ctx, const clang::T##Decl* decl) -> void

#define MAP(T) fn map(Context& ctx, T entity) -> Node<Type>

fn wrap(Context& ctx, const clang::Decl* decl) -> void;
MAP(const clang::Decl*);
MAP(const clang::Type*);

// Maps to an atomic type of some kind. Many of these are obscure and
// unsupported.
MAP(const clang::BuiltinType*) {
   switch (entity->getKind()) {
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
      entity->dump();
      fatal("unhandled builtin type: ",
            entity->getNameAsCString(clang::PrintingPolicy(clang::LangOptions())));
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
      entity->dump();
      fatal("unreachable builtin type: ",
            entity->getNameAsCString(clang::PrintingPolicy(clang::LangOptions())));
   }
} // namespace ensnare::ct

MAP(clang::QualType) {
   // Too niche to care about for now.
   if (entity.isRestrictQualified() || entity.isVolatileQualified()) {
      fatal("volatile and restrict qualifiers unsupported");
   }
   auto result = map(ctx, entity.getTypePtr());
   if (entity.isConstQualified()) { // FIXME: `isLocalConstQualified`: do
                                    // we care about local vs non-local?
                                    // This will be usefull later.
                                    // result->flag_const();
   }
   return result;
}

MAP(Node<TypeDecl>) {
   if (is<AliasTypeDecl>(entity)) {
      return node<Type>(AtomType(deref<AliasTypeDecl>(entity).name()));
   } else if (is<EnumTypeDecl>(entity)) {
      return node<Type>(AtomType(deref<EnumTypeDecl>(entity).name()));
   } else if (is<RecordTypeDecl>(entity)) {
      return node<Type>(AtomType(deref<RecordTypeDecl>(entity).name()));
   } else {
      fatal("unreachable");
   }
}

MAP(const clang::ElaboratedType*) {
   return map(ctx, entity->getNamedType());
}

MAP(const clang::NamedDecl*) {
   auto type = ctx.lookup(entity);
   if (type) {
      return *type;
   } else {
      wrap(ctx, entity);
      auto type = ctx.lookup(entity);
      if (type) {
         return *type;
      } else {
         entity->dump();
         fatal("failed to map type");
      }
   }
}

// FIXME: add the restrict_wrap counter.

// If a `Type` has a `Decl` representation always map that. It seems to work.
MAP(const clang::TypedefType*) {
   return map(ctx, entity->getDecl());
}

MAP(const clang::RecordType*) {
   return map(ctx, entity->getDecl());
}

MAP(const clang::PointerType*) {
   return node<Type>(map(ctx, entity->getPointeeType()));
}

MAP(const clang::VectorType*) {
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

// return a suitable name from a named declaration.
fn nim_name(Context& ctx, const clang::NamedDecl* decl) -> Str {
   auto name = decl->getDeclName();
   switch (name.getNameKind()) {
   case clang::DeclarationName::NameKind::Identifier: {
      return decl->getNameAsString();
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
      decl->dump();
      fatal("unhandled decl name");
   }
}

// Retrieve all the information we have about a declaration.
template <typename T> fn get_definition(const T* decl) -> const T* {
   if (auto def = decl->getDefinition()) {
      return def;
   } else {
      return decl;
   }
}

WRAP(Record) {
   // auto def = get_definition(decl);
   RecordTypeDecl record(nim_name(ctx, decl), decl->getQualifiedNameAsString());
   auto type_decl = node<TypeDecl>(record);
   ctx.set(decl, map(ctx, type_decl));
   ctx.add(type_decl);
}

WRAP(TypedefName) {
   if (decl->getKind() == clang::Decl::Kind::ObjCTypeParam) {
      fatal("obj-c is unsupported");
   } else {
      // This could be c++ `TypeAliasDecl` or a `TypedefDecl`.
      // Either way, we produce `type AliasName* = UnderlyingType`
      AliasTypeDecl alias(decl->getNameAsString(), map(ctx, decl->getUnderlyingType()));
      auto type_decl = node<TypeDecl>(alias);
      ctx.set(decl, map(ctx, type_decl));
      ctx.add(type_decl);
   }
}

fn wrap_non_template(Context& ctx, const clang::FunctionDecl* decl) {
   FreeFunctionDecl free_func(decl->getNameAsString());
   auto func = node<FunctionDecl>(free_func);
   ctx.add(func);
}

WRAP(Var) {
   ctx.add(node<VariableDecl>(decl->getNameAsString(), decl->getQualifiedNameAsString(),
                              map(ctx, decl->getType())));
}

WRAP(Function) {
   // This is just a plain function. Not a method of some kind.
   // It could be templated though.
   switch (decl->getTemplatedKind()) {
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

// --- dispatching, presumably the hot path.

#define DISPATCH(kind)                                                                             \
   case clang::Type::TypeClass::kind: {                                                            \
      return map(ctx, llvm::cast<clang::kind##Type>(entity));                                      \
   }

MAP(const clang::Type*) {
   switch (entity->getTypeClass()) {
      DISPATCH(Builtin);
      DISPATCH(Elaborated);
      DISPATCH(Record);
      DISPATCH(Pointer);
      DISPATCH(Typedef);
      DISPATCH(Vector);
   default:
      entity->dump();
      fatal("unhandled mapping: ", entity->getTypeClassName());
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

fn wrap(Context& ctx, const clang::Decl* decl) -> void {
   switch (decl->getKind()) {
      DISCARD(TranslationUnit);
      DISCARD(Namespace);
      DISCARD(Field);
      DISCARD(ParmVar);
      DISPATCH_ANY(TypedefName);
      DISPATCH_ANY(Record);
      DISPATCH(Function);
      DISPATCH(Var);
   default:
      decl->dump();
      fatal("unhandled wrapping: ", decl->getDeclKindName(), "Decl");
   }
}

class Visitor : public clang::RecursiveASTVisitor<Visitor> {
   priv Context& ctx;
   priv const clang::ASTContext& ast_ctx;

   pub Visitor(Context& ctx, const clang::ASTContext& ast_ctx) : ctx(ctx), ast_ctx(ast_ctx) {}

   pub fn VisitDecl(clang::Decl* decl) -> bool {
      wrap(this->ctx, decl);
      return true;
   }
};

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

fn render(const Node<Type>& type) -> Str;

fn render(const Node<Sym>& sym) -> Str {
   return sym->latest();
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

fn render_colon(const Str& a, const Str& b) -> Str {
   return a + ": " + b;
}

fn import_cpp(const Str& pattern) -> Str {
   return "import_cpp: \"" + pattern + "\"";
}

fn render(const AliasTypeDecl& decl) -> Str {
   return render(decl.name()) + "* = " + render(decl.type());
}

fn render(const EnumFieldDecl& decl) -> Str {
   return indent() + render(decl.name()) + '\n';
}

fn render(const EnumTypeDecl& decl) -> Str {
   Str result =
       render(decl.name()) + "* " + render_pragmas({import_cpp(decl.cpp_name())}) + " = enum\n";
   for (const auto& field : decl.fields()) {
      result += render(field);
   }
   return result;
}

fn render(const RecordTypeDecl& decl) -> Str {
   return "FIXME\n";
}

fn render(const Node<TypeDecl>& decl) -> Str {
   if (is<AliasTypeDecl>(decl)) {
      return rende(deref<AliasTypeDecl>(decl));
   } else if (is<EnumTypeDecl>(decl)) {
      return render(deref<EnumTypeDecl>(decl));
   } else if (is<RecordTypeDecl>(decl)) {
      return render(deref<RecordTypeDecl>(decl));
   } else {
      fatal("unreachable");
   }
}

fn render(const Node<FunctionDecl>& decl) -> Str {
   return "FIXME\n";
}

fn render(const Node<VariableDecl>& decl) -> Str {
   return "FIXME\n";
}

fn render(const Node<Type>& type) -> Str {
   return "FIXME\n";
}
} // namespace rendering

fn finalize(const Context& ctx) {
   Str output;
   output += "# ensnare generated wrapper\n";
   output += "\n";
   for (const auto& type_decl : ctx.type_decls()) {
      output += rendering::render(type_decl);
   }
   for (const auto& function_decl : ctx.function_decls()) {
      output += rendering::render(function_decl);
   }
   for (const auto& variable_decl : ctx.variable_decls()) {
      output += rendering::render(variable_decl);
   }
   os::write_file(ctx.cfg.name() + ".nim", output);
}

fn run(int argc, const char* argv[]) -> bool {
   Config cfg(argc, argv);
   auto tu = parse_translation_unit(cfg);
   Context ctx(cfg);
   Visitor visitor(ctx, tu->getASTContext());
   auto result = visitor.TraverseAST(tu->getASTContext());
   if (!result) {
      return false;
   } else {
      finalize(ctx);
      return true;
   }
}
} // namespace ensnare::ct

#include "private/undef_syn.hpp"


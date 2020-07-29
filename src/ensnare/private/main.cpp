#include "ensnare/private/main.hpp"

#include "ensnare/private/bit_utils.hpp"
#include "ensnare/private/builtins.hpp"
#include "ensnare/private/clang_utils.hpp"
#include "ensnare/private/config.hpp"
#include "ensnare/private/headers.hpp"
#include "ensnare/private/ir.hpp"
#include "ensnare/private/os_utils.hpp"
#include "ensnare/private/render.hpp"
#include "ensnare/private/runtime.hpp"
#include "ensnare/private/str_utils.hpp"
#include "ensnare/private/utils.hpp"

#include "llvm/ADT/StringMap.h"

#include <random>

// FIXME: move to os utils.
#ifdef __ARM_ARCH_ISA_A64
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
class SymGenerator {
   priv std::mt19937 rng;
   priv Vec<char> chars;
   priv std::uniform_int_distribution<std::size_t> sampler;

   priv fn seed() -> std::random_device::result_type {
      std::random_device entropy;
      return entropy();
   }

   priv fn init_chars() -> Vec<char> {
      Vec<char> result;
      for (auto i = static_cast<int>('a'); i < static_cast<int>('z'); i += 1) {
         result.push_back(static_cast<char>(i));
      }
      for (auto i = static_cast<int>('A'); i < static_cast<int>('Z'); i += 1) {
         result.push_back(static_cast<char>(i));
      }
      for (auto i = static_cast<int>('0'); i < static_cast<int>('1'); i += 1) {
         result.push_back(static_cast<char>(i));
      }
      return result;
   }

   pub SymGenerator() : rng(seed()), chars(init_chars()), sampler(0, chars.size() - 1) {}

   priv fn sample_char() -> char { return chars[sampler(rng)]; }

   pub fn operator()(std::size_t size) -> Str {
      Str result;
      for (auto i = 0; i < size; i += 1) {
         result.push_back(sample_char());
      }
      return result;
   }
};

class HeaderCanonicalizer {
   const clang::SourceManager& source_manager;
   const Vec<Str> headers;

   using IncludeDirMap = llvm::StringMap<Str>;
   IncludeDirMap include_dir_map;

   fn init_include_dir_map(const Config& cfg) const -> IncludeDirMap {
      IncludeDirMap result;
      for (const auto& unprocessed_dir : cfg.include_dirs()) {
         auto dir = clang::tooling::getAbsolutePath(unprocessed_dir);
         for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (fs::is_regular_file(entry)) {
               // FIXME: maybe we should swtich to LLVM's path handling...
               auto str_entry = Str(fs::path(entry));
               if (result.count(str_entry) == 0 || dir.size() > result[str_entry].size()) {
                  result[str_entry] = dir;
               }
            }
         }
      }
      return result;
   }

   fn init_headers(const Config& cfg) const -> Vec<Str> {
      Vec<Str> result;
      for (const auto& header : cfg.headers()) {
         result.push_back(header);
      }
      return result;
   }

   pub HeaderCanonicalizer(const Config& cfg, const clang::SourceManager& source_manager)
      : source_manager(source_manager),
        headers(init_headers(cfg)),
        include_dir_map(init_include_dir_map(cfg)) {}

   pub fn operator[](const clang::SourceLocation& loc) -> Opt<Str> {
      auto path = clang::tooling::getAbsolutePath(source_manager.getFilename(loc));
      for (auto const& header : headers) {
         if (ends_with(path, header)) {
            return header;
         }
      }
      if (include_dir_map.count(path) != 0) {
         return path.substr(include_dir_map[path].size() + 1);
      } else {
         return {};
      }
   }
};

/// Holds ensnare's state.
///
/// A `Context` must not outlive a `Config`
class Context {
   pub const clang::ASTContext& ast_ctx;
   priv Map<const clang::Decl*, Node<Type>>
       type_lookup; ///< For mapping bound declarations to exisiting types.

   priv Vec<Node<TypeDecl>> _type_decls;         ///< Types to output.
   priv Vec<Node<RoutineDecl>> _routine_decls;   ///< Functions to output.
   priv Vec<Node<VariableDecl>> _variable_decls; ///< Variables to output.

   priv HeaderCanonicalizer header_canonicalizer;

   pub fn type_decls() const -> const Vec<Node<TypeDecl>>& { return _type_decls; }
   pub fn routine_decls() const -> const Vec<Node<RoutineDecl>>& { return _routine_decls; }
   pub fn variable_decls() const -> const Vec<Node<VariableDecl>>& { return _variable_decls; }

   priv Vec<const clang::NamedDecl*> decl_stack; ///< To give anonymous tags useful names, we track
                                                 ///< the declaration they were referenced from.
   pub const Config& cfg;

   pub Context(const Config& cfg, const clang::ASTContext& ast_ctx)
      : cfg(cfg), ast_ctx(ast_ctx), header_canonicalizer(cfg, ast_ctx.getSourceManager()) {}

   pub fn decl(int i) const -> const clang::NamedDecl& {
      if (i < decl_stack.size()) {
         return *decl_stack[decl_stack.size() - i - 1];
      } else {
         for (auto decl : decl_stack) {
            print(render(decl));
         }
         fatal("unreachable: failed to get decl context");
      }
   }
   pub fn push(const clang::NamedDecl& decl) { decl_stack.push_back(&decl); }
   pub fn pop_decl() { decl_stack.pop_back(); }

   fn canon_lookup_decl(const clang::Decl& decl) const -> const clang::Decl& {
      if (auto tag_decl = llvm::dyn_cast<clang::TagDecl>(&decl)) {
         return get_definition(*tag_decl);
      } else {
         return decl;
      }
   }

   /// Lookup the Node<Type> of a declaration.
   pub fn lookup(const clang::Decl& decl) const -> Opt<Node<Type>> {
      auto decl_type = type_lookup.find(&canon_lookup_decl(decl));
      if (decl_type == type_lookup.end()) {
         return {};
      } else {
         return decl_type->second;
      }
   }

   /// Record the Node<Type> of a declaration.
   pub fn associate(const clang::Decl& decl, const Node<Type> type) {
      auto& key = canon_lookup_decl(decl);
      auto maybe_type = lookup(key);
      if (maybe_type) {
         print(render(decl));
         fatal("unreachable: duplicate type in lookup");
      } else {
         type_lookup[&key] = type;
      }
   }

   /// Add a type declaration to be rendered.
   pub fn add(const Node<TypeDecl> decl) { _type_decls.push_back(decl); }

   /// Add a routine declaration to be rendered.
   pub fn add(const Node<RoutineDecl> decl) { _routine_decls.push_back(decl); }

   /// Add a variable declaration to be rendered.
   pub fn add(const Node<VariableDecl> decl) { _variable_decls.push_back(decl); }

   /// Filters access to protected and private members.
   pub fn access_guard(const clang::Decl& decl) const -> bool {
      return decl.getAccess() == clang::AS_public || decl.getAccess() == clang::AS_none;
   }

   pub fn maybe_header(const clang::NamedDecl& decl) -> Opt<Str> {
      return header_canonicalizer[decl.getLocation()];
   }

   pub fn header(const clang::NamedDecl& decl) -> Str {
      auto h = maybe_header(decl);
      if (h) {
         return *h;
      } else {
         print(render(decl));
         fatal("failed to canonicalize source location");
      }
   }
};

fn force_wrap(Context& ctx, const clang::NamedDecl& decl) -> void;

fn map(Context& ctx, const clang::Type& decl) -> Node<Type>;

/// Generic map for pointers to error on null.
template <typename T> fn map(Context& ctx, const T* entity) -> Node<Type> {
   if (entity) {
      return map(ctx, *entity);
   } else {
      fatal("tried to map a null entity");
   }
}

/// Maps to an atomic type of some kind. Many of these are obscure and unsupported.
fn map(Context& ctx, const clang::BuiltinType& entity) -> Node<Type> {
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
      case clang::BuiltinType::Kind::OCLReserveID: fatal("opencl builtins unsupported");
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
      case clang::BuiltinType::Kind::SveBool: fatal("SVE builtins unsupported");
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
      case clang::BuiltinType::Kind::SatULongFract:
         fatal("Sat, Fract, and  Accum builtins unsupported");
      case clang::BuiltinType::Kind::ObjCId:
      case clang::BuiltinType::Kind::ObjCClass:
      case clang::BuiltinType::Kind::ObjCSel: fatal("obj-c builtins unsupported");
      case clang::BuiltinType::Kind::Char_S: // 'char' for targets where it's signed.
         fatal("this should be unreachable because nim compiles with unsigned chars");
      case clang::BuiltinType::Kind::Dependent:
      case clang::BuiltinType::Kind::Overload:
      case clang::BuiltinType::Kind::BoundMember:
      case clang::BuiltinType::Kind::PseudoObject:
      case clang::BuiltinType::Kind::UnknownAny:
      case clang::BuiltinType::Kind::ARCUnbridgedCast:
      case clang::BuiltinType::Kind::BuiltinFn:
      case clang::BuiltinType::Kind::OMPArraySection:
         entity.dump();
         fatal("unhandled builtin type: ",
               entity.getNameAsCString(clang::PrintingPolicy(clang::LangOptions())));
      case clang::BuiltinType::Kind::SChar: return builtins::_schar;
      case clang::BuiltinType::Kind::Short: return builtins::_short;
      case clang::BuiltinType::Kind::Int: return builtins::_int;
      case clang::BuiltinType::Kind::Long: return builtins::_long;
      case clang::BuiltinType::Kind::LongLong: return builtins::_long_long;
      case clang::BuiltinType::Kind::UChar: return builtins::_char;
      case clang::BuiltinType::Kind::UShort: return builtins::_ushort;
      case clang::BuiltinType::Kind::UInt: return builtins::_uint;
      case clang::BuiltinType::Kind::ULong: return builtins::_ulong;
      case clang::BuiltinType::Kind::ULongLong: return builtins::_ulong_long;
      case clang::BuiltinType::Kind::Char_U: return builtins::_char;
      case clang::BuiltinType::Kind::WChar_U:
      case clang::BuiltinType::Kind::WChar_S: return builtins::_wchar;
      case clang::BuiltinType::Kind::Char8: return builtins::_char8;
      case clang::BuiltinType::Kind::Char16: return builtins::_char16;
      case clang::BuiltinType::Kind::Char32: return builtins::_char32;
      case clang::BuiltinType::Kind::Bool: return builtins::_bool;
      case clang::BuiltinType::Kind::Float: return builtins::_float;
      case clang::BuiltinType::Kind::Double: return builtins::_double;
      case clang::BuiltinType::Kind::LongDouble: return builtins::_long_double;
      case clang::BuiltinType::Kind::Void: return builtins::_void;
      case clang::BuiltinType::Kind::Int128: return builtins::_int128;
      case clang::BuiltinType::Kind::UInt128: return builtins::_uint128;
      case clang::BuiltinType::Kind::Half: return builtins::_neon_float16;
      case clang::BuiltinType::Kind::Float16: return builtins::_float16;
      case clang::BuiltinType::Kind::Float128: return builtins::_float128;
      case clang::BuiltinType::Kind::NullPtr: return builtins::_nullptr;
      default:
         entity.dump();
         fatal("unreachable: builtin type: ",
               entity.getNameAsCString(clang::PrintingPolicy(clang::LangOptions())));
   }
}

/// Map a const/volatile/__restrict qualified type.
fn map(Context& ctx, const clang::QualType& entity) -> Node<Type> {
   // Too niche to care about for now.
   // if (entity.isRestrictQualified() || entity.isVolatileQualified()) {
   //   fatal("volatile and restrict qualifiers unsupported");
   //}
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

/// This maps certain kinds of declarations by first trying to look them up in the context.
/// If it is not in the context, we enforce wrapping it and return the mapped result.
fn map(Context& ctx, const clang::NamedDecl& decl) -> Node<Type> {
   auto type = ctx.lookup(decl);
   if (type) {
      return *type;
   } else {
      force_wrap(ctx, decl);
      auto type = ctx.lookup(decl);
      if (type) {
         return *type;
      } else {
         decl.dump();
         fatal("unreachable: failed to map type");
      }
   }
}

fn map(Context& ctx, const clang::Type& type) -> Node<Type> {
   switch (type.getTypeClass()) {
      case clang::Type::TypeClass::Elaborated:
         // Map an elaborated type. An elaborated type is a syntactic node to not lose information
         // in the clang AST. We care about semantics so we can ignore this.
         return map(ctx, llvm::cast<clang::ElaboratedType>(type).getNamedType());
      // If something has a decl representation, map that.
      case clang::Type::TypeClass::Typedef:
         return map(ctx, llvm::cast<clang::TypedefType>(type).getDecl());
      case clang::Type::TypeClass::Record:
         return map(ctx, get_definition(llvm::cast<clang::RecordType>(type).getDecl()));
      case clang::Type::TypeClass::Enum:
         return map(ctx, get_definition(llvm::cast<clang::EnumType>(type).getDecl()));
      case clang::Type::TypeClass::Pointer: {
         const auto& ty = llvm::cast<clang::PointerType>(type);
         // FIXME: fix function pointers.
         if (ty.isVoidPointerType()) {
            return builtins::_void_ptr;
         } else {
            return node<Type>(PtrType(map(ctx, ty.getPointeeType())));
         }
      }
      case clang::Type::TypeClass::LValueReference:
      case clang::Type::TypeClass::RValueReference:
         return node<Type>(
             RefType(map(ctx, llvm::cast<clang::ReferenceType>(type).getPointeeType())));
      case clang::Type::TypeClass::Vector:
         // The hope is that this is an aliased type as part of `typedef` / `using`
         // and the internals don't matter.
         return node<Type>(OpaqueType());
      case clang::Type::TypeClass::ConstantArray: {
         const auto& ty = llvm::cast<clang::ConstantArrayType>(type);
         return node<Type>(
             ArrayType(ty.getSize().getLimitedValue(), map(ctx, ty.getElementType())));
      }
      case clang::Type::TypeClass::IncompleteArray:
         return node<Type>(UnsizedArrayType(
             map(ctx, llvm::cast<clang::IncompleteArrayType>(type).getElementType())));
      case clang::Type::TypeClass::FunctionProto: {
         const auto& ty = llvm::cast<clang::FunctionProtoType>(type);
         Vec<Node<Type>> types;
         for (auto param_type : ty.param_types()) {
            types.push_back(map(ctx, param_type));
         }
         return node<Type>(FuncType(types, map(ctx, ty.getReturnType())));
      }
      case clang::Type::TypeClass::Decltype:
         return map(ctx, llvm::cast<clang::DecltypeType>(type).getUnderlyingType());
      case clang::Type::TypeClass::Paren:
         return map(ctx, llvm::cast<clang::ParenType>(type).getInnerType());
      case clang::Type::TypeClass::Builtin: return map(ctx, llvm::cast<clang::BuiltinType>(type));
      default: type.dump(); fatal("unhandled mapping: ", type.getTypeClassName());
   }
}

fn transfer(Context& ctx, const clang::ParmVarDecl& param) -> ParamDecl {
   return ParamDecl(param.getNameAsString(), map(ctx, param.getType()));
}

fn transfer(Context& ctx, const clang::FunctionDecl& decl) -> Vec<ParamDecl> {
   Vec<ParamDecl> result;
   for (auto param : decl.parameters()) {
      result.push_back(transfer(ctx, *param));
   }
   return result;
}

/// Map the return type if it has one.
fn transfer_return_type(Context& ctx, const clang::FunctionDecl& decl) -> Opt<Node<Type>> {
   auto result = decl.getReturnType();
   if (result->isVoidType()) {
      return {};
   } else {
      return map(ctx, result);
   }
}

fn wrap_non_template(Context& ctx, const clang::FunctionDecl& decl) {
   ctx.add(node<RoutineDecl>(FunctionDecl(decl.getNameAsString(), qual_name(decl), ctx.header(decl),
                                          transfer(ctx, decl), transfer_return_type(ctx, decl))));
}

fn assert_non_template(const clang::FunctionDecl& decl) {
   switch (decl.getTemplatedKind()) {
      case clang::FunctionDecl::TK_NonTemplate: break;
      case clang::FunctionDecl::TK_FunctionTemplate:
      case clang::FunctionDecl::TK_MemberSpecialization:
      case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization:
         fatal("only simple function signatures are supported.");
   }
}

fn wrap_function(Context& ctx, const clang::FunctionDecl& decl) {
   assert_non_template(decl);
   wrap_non_template(ctx, decl);
}

fn wrap_non_template(Context& ctx, const clang::CXXConstructorDecl& decl) {
   // We don't wrap anonymous constructors since they take a supposedly anonymous type as a
   // parameter.
   if (ctx.access_guard(decl) && has_name(*decl.getParent())) {
      ctx.add(node<RoutineDecl>(ConstructorDecl(qual_name(decl), ctx.header(decl),
                                                map(ctx, decl.getParent()), transfer(ctx, decl))));
   }
}

fn wrap(Context& ctx, const clang::CXXConstructorDecl& decl) {
   assert_non_template(decl);
   wrap_non_template(ctx, decl);
}

fn wrap_non_template(Context& ctx, const clang::CXXMethodDecl& decl) {
   if (ctx.access_guard(decl)) {
      ctx.add(node<RoutineDecl>(MethodDecl(decl.getNameAsString(), qual_name(decl),
                                           ctx.header(decl), map(ctx, decl.getParent()),
                                           transfer(ctx, decl), transfer_return_type(ctx, decl))));
   }
}

fn wrap(Context& ctx, const clang::CXXMethodDecl& decl) {
   assert_non_template(decl);
   wrap_non_template(ctx, decl);
}

fn wrap_methods(Context& ctx, const clang::CXXRecordDecl::method_range methods) {
   for (auto method : methods) {
      // manual check because isa considers descendents too.
      if (method->getKind() == clang::Decl::Kind::CXXMethod) {
         wrap(ctx, *method);
      } else if (auto constructor = llvm::dyn_cast<clang::CXXConstructorDecl>(method)) {
         wrap(ctx, *constructor);
      } else if (auto conversion = llvm::dyn_cast<clang::CXXConversionDecl>(method)) {
         wrap(ctx, *conversion);
      } else if (auto destructor = llvm::dyn_cast<clang::CXXDestructorDecl>(method)) {
         // We don't wrap destructors to make it harder for fools to call them manually.
         // wrap(ctx, *destructor);
      } else {
         fatal("unreachable: wrap(CXXRecordDecl::method_range)");
      }
   }
}

/// A qualified name contains all opaque namespace and type contexts.
/// Instead of the c++ double colon we use a minus and stropping.
fn qual_nim_name(Context& ctx, const clang::NamedDecl& decl) -> Str {
   return replace(qual_name(decl), "::", "-");
}

fn transfer(Context& ctx, const clang::CXXRecordDecl::field_range fields) -> Vec<RecordFieldDecl> {
   Vec<RecordFieldDecl> result;
   for (const auto field : fields) {
      if (ctx.access_guard(*field)) {
         ctx.push(*field);
         result.push_back(RecordFieldDecl(field->getNameAsString(), map(ctx, field->getType())));
         ctx.pop_decl();
      }
   }
   return result;
}

fn tag_name(Context& ctx, const clang::NamedDecl& decl) -> Node<Sym> {
   return has_name(decl) ? node<Sym>(qual_nim_name(ctx, decl))
                         : node<Sym>("type_of(" + qual_nim_name(ctx, ctx.decl(1)) + ")");
}

fn register_tag_name(Context& ctx, const clang::NamedDecl& name_decl,
                     const clang::TagDecl& def_decl)
    ->Node<Sym> {
   auto result = tag_name(ctx, name_decl);
   auto type = node<Type>(result);
   ctx.associate(name_decl, type);
   if (!eq_ref(name_decl, llvm::cast<clang::NamedDecl>(def_decl))) {
      ctx.associate(def_decl, type);
   }
   return result;
}

fn tag_import_name(Context& ctx, const clang::NamedDecl& decl) -> Str {
   return has_name(decl) ? decl.getQualifiedNameAsString()
                         : "decltype(" + ctx.decl(1).getQualifiedNameAsString() + ")";
}

fn wrap_record(Context& ctx, const clang::NamedDecl& name_decl, const clang::RecordDecl& def_decl,
               bool force) {
   if (has_name(name_decl) || force) {
      ctx.add(node<TypeDecl>(RecordTypeDecl(register_tag_name(ctx, name_decl, def_decl),
                                            tag_import_name(ctx, name_decl), ctx.header(name_decl),
                                            transfer(ctx, def_decl.fields()))));
      if (def_decl.getKind() == clang::Decl::Kind::CXXRecord && !force) {
         wrap_methods(ctx, llvm::cast<clang::CXXRecordDecl>(def_decl).methods());
      }
   }
}

fn wrap_enum(Context& ctx, const clang::NamedDecl& name_decl, const clang::EnumDecl& def_decl,
             bool force) {
   if (has_name(name_decl) || force) {
      Vec<EnumFieldDecl> fields;
      for (auto field : def_decl.enumerators()) {
         // FIXME: expose if a value was explicitly provided.
         // auto expr = field->getInitExpr();
         // if (expr) {
         // } else {
         // }
         fields.push_back(
             EnumFieldDecl(field->getNameAsString(), field->getInitVal().getExtValue()));
      }
      ctx.add(node<TypeDecl>(EnumTypeDecl(register_tag_name(ctx, name_decl, def_decl),
                                          tag_import_name(ctx, name_decl), ctx.header(name_decl),
                                          fields)));
   }
}

fn wrap_tag(Context& ctx, const clang::NamedDecl& name_decl, const clang::TagDecl& def_decl,
            bool force) {
   auto kind = def_decl.getKind();
   if (kind == clang::Decl::Kind::Record || kind == clang::Decl::Kind::CXXRecord) {
      wrap_record(ctx, name_decl, llvm::cast<clang::RecordDecl>(def_decl), force);
   } else if (kind == clang::Decl::Kind::Enum) {
      wrap_enum(ctx, name_decl, llvm::cast<clang::EnumDecl>(def_decl), force);
   } else {
      fatal("unhandled: anon tag typedef");
   }
   // wrap_tag(ctx, register_tag_name(ctx, name_decl, def_decl), name_decl, def_decl, force);
}

fn wrap_tag(Context& ctx, const clang::TagDecl& def_decl, bool force) {
   wrap_tag(ctx, def_decl, def_decl, force);
}

/// Check if `decl` is from <cstddef>.
fn get_cstddef_item(Context& ctx, const clang::NamedDecl& decl) -> Opt<Node<Type>> {
   auto qualified_name = decl.getQualifiedNameAsString();
   if (qualified_name == "std::size_t" || qualified_name == "size_t") {
      return builtins::_size;
   } else if (qualified_name == "std::ptrdiff_t" || qualified_name == "ptrdiff_t") {
      return builtins::_ptrdiff;
   } else if (qualified_name == "std::nullptr_t") {
      return builtins::_nullptr;
   } else {
      return {};
   }
}

fn wrap_simple_alias(Context& ctx, const clang::TypedefNameDecl& decl) {
   auto name = node<Sym>(qual_nim_name(ctx, decl));
   ctx.associate(decl, node<Type>(name));
   auto type_decl = node<TypeDecl>(AliasTypeDecl(name, map(ctx, decl.getUnderlyingType())));
   ctx.add(type_decl);
}

fn t_suffix_eq(Context& ctx, const clang::NamedDecl& type_t, const Str& type) -> bool {
   return ctx.cfg.fold_type_suffix() && qual_name(type_t) == type + "_t";
}

fn add_t_suffix(Context& ctx, const clang::TypedefDecl& decl) {
   auto type = map(ctx, decl);
   if (is<Node<Sym>>(type)) {
      auto name = deref<Node<Sym>>(type);
      if (!ends_with(name->latest(), "_t")) {
         name->update(name->latest() + "_t");
      }
   } else {
      fatal("unreachable: add_t_suffix");
   }
}

fn wrap_tag_typedef(Context& ctx, const clang::TypedefDecl& name_decl,
                    const clang::TagDecl& def_decl, bool owns_tag) {
   if (has_name(def_decl)) {
      if (eq_ident(name_decl, def_decl)) {
         // typedef struct Foo {} Foo; ->
         // type Foo {.import_cpp: "Foo".} = object
         if (auto type = ctx.lookup(def_decl)) {
            ctx.associate(name_decl, *type);
         } else {
            wrap_tag(ctx, name_decl, def_decl, true);
         }
      } else if (t_suffix_eq(ctx, name_decl, qual_name(def_decl))) {
         if (auto type = ctx.lookup(def_decl)) {
            ctx.associate(name_decl, *type);
         } else {
            wrap_tag(ctx, name_decl, def_decl, true);
         }
         add_t_suffix(ctx, name_decl);
      } else {
         // typedef struct Bar {} Foo; ->
         // type Bar {.import_cpp: "Bar".} = object
         // type Foo = Bar
         wrap_simple_alias(ctx, name_decl);
      }
   } else if (owns_tag) {
      // typdef struct {} Foo;
      // type Foo {.import_cpp: "Foo".} = object
      wrap_tag(ctx, name_decl, def_decl, true);
   } else {
      wrap_simple_alias(ctx, name_decl);
   }
}

fn wrap_typedef(Context& ctx, const clang::TypedefDecl& decl) {
   if (auto tag_decl = owns_tag(decl)) {
      // A typedef with an inline tag declaration.
      wrap_tag_typedef(ctx, decl, *tag_decl, true);
   } else if (auto tag_decl = inner_tag(decl)) {
      // A typedef with a that refs a tag declared elsewhere.
      wrap_tag_typedef(ctx, decl, *tag_decl, false);
   } else if (const auto cstddef_item = get_cstddef_item(ctx, decl)) {
      // References to the typedef should fold to our builtins.
      ctx.associate(decl, *cstddef_item);
   } else {
      wrap_simple_alias(ctx, decl);
   }
}

fn force_wrap(Context& ctx, const clang::NamedDecl& named_decl) -> void {
   if (!ctx.lookup(named_decl)) {
      log("force wrap", named_decl);
      ctx.push(named_decl);
      switch (named_decl.getKind()) {
         case clang::Decl::Kind::Enum:
         case clang::Decl::Kind::Record:
         case clang::Decl::Kind::CXXRecord:
            wrap_tag(ctx, get_definition(llvm::cast<clang::TagDecl>(named_decl)), true);
            break;
         case clang::Decl::Kind::TypeAlias:
            wrap_simple_alias(ctx, llvm::cast<clang::TypeAliasDecl>(named_decl));
            break;
         case clang::Decl::Kind::Typedef:
            wrap_typedef(ctx, llvm::cast<clang::TypedefDecl>(named_decl));
            break;
         default: fatal("unhandled force_wrap: ", named_decl.getDeclKindName(), "Decl");
      }
      ctx.pop_decl();
   }
}

fn wrap(Context& ctx, const clang::NamedDecl& named_decl) -> void {
   if (!ctx.lookup(named_decl) && ctx.maybe_header(named_decl)) {
      log("wrap", named_decl);
      ctx.push(named_decl);
      switch (named_decl.getKind()) {
         case clang::Decl::Kind::TypeAlias:
            wrap_simple_alias(ctx, llvm::cast<clang::TypeAliasDecl>(named_decl));
            break;
         case clang::Decl::Kind::Typedef:
            wrap_typedef(ctx, llvm::cast<clang::TypedefDecl>(named_decl));
            break;
         case clang::Decl::Kind::Enum:
         case clang::Decl::Kind::Record:
         case clang::Decl::Kind::CXXRecord:
            wrap_tag(ctx, get_definition(llvm::cast<clang::TagDecl>(named_decl)), false);
            break;
         case clang::Decl::Kind::Function:
            wrap_function(ctx, llvm::cast<clang::FunctionDecl>(named_decl));
            break;
         case clang::Decl::Kind::Var: {
            auto& decl = llvm::cast<clang::VarDecl>(named_decl);
            if (ctx.access_guard(decl) && decl.hasGlobalStorage() && !decl.isStaticLocal()) {
               ctx.add(node<VariableDecl>(qual_nim_name(ctx, decl), qual_name(decl),
                                          ctx.header(decl), map(ctx, decl.getType())));
            }
            break;
         }
         case clang::Decl::Kind::Namespace: // FIXME: doing something with this would be good.
         case clang::Decl::Kind::NamespaceAlias:
         case clang::Decl::Kind::Field:
         case clang::Decl::Kind::ParmVar:
         case clang::Decl::Kind::Using:
         case clang::Decl::Kind::EnumConstant:
         case clang::Decl::Kind::CXXMethod:
         case clang::Decl::Kind::CXXConstructor:
         case clang::Decl::Kind::CXXDestructor: {
            break; // discarded
         }
         default: fatal("unhandled wrapping: ", named_decl.getDeclKindName(), "Decl");
      }
      ctx.pop_decl();
   }
}

/// The base wrapping routine that only filters out unnamed decls.
fn base_wrap(Context& ctx, const clang::Decl& decl) {
   switch (decl.getKind()) {
      case clang::Decl::Kind::AccessSpec:
      case clang::Decl::Kind::Block:
      case clang::Decl::Kind::Captured:
      case clang::Decl::Kind::ClassScopeFunctionSpecialization:
      case clang::Decl::Kind::Empty:
      case clang::Decl::Kind::Export:
      case clang::Decl::Kind::ExternCContext:
      case clang::Decl::Kind::FileScopeAsm:
      case clang::Decl::Kind::Friend:
      case clang::Decl::Kind::FriendTemplate:
      case clang::Decl::Kind::Import:
      case clang::Decl::Kind::LifetimeExtendedTemporary:
      case clang::Decl::Kind::LinkageSpec:
      case clang::Decl::Kind::ObjCPropertyImpl:
      case clang::Decl::Kind::OMPAllocate:
      case clang::Decl::Kind::OMPRequires:
      case clang::Decl::Kind::OMPThreadPrivate:
      case clang::Decl::Kind::PragmaComment:
      case clang::Decl::Kind::PragmaDetectMismatch:
      case clang::Decl::Kind::RequiresExprBody:
      case clang::Decl::Kind::StaticAssert:
      case clang::Decl::Kind::TranslationUnit: {
         break; // discarded
      }
      case clang::Decl::Kind::firstNamed... clang::Decl::Kind::lastNamed: {
         wrap(ctx, llvm::cast<clang::NamedDecl>(decl));
         break;
      }
      default: decl.dump(); fatal("unhandled wrapping: ", decl.getDeclKindName(), "Decl");
   }
}

fn prefixed_search_paths() -> Vec<Str> {
   Vec<Str> result;
   for (const auto& search_path : Header::search_paths()) {
      result.push_back("-isystem" + search_path);
   }
   return result;
}

/// Load a translation unit from user provided arguments with additional include path arguments.
fn parse_translation_unit(const Config& cfg) -> std::unique_ptr<clang::ASTUnit> {
   Vec<Str> args = {"-xc++"};
   auto search_paths = prefixed_search_paths();
   args.insert(args.end(), search_paths.begin(), search_paths.end());
   // The users args get placed after for higher priority.
   args.insert(args.end(), cfg.user_clang_args().begin(), cfg.user_clang_args().end());
   return clang::tooling::buildASTFromCodeWithArgs(cfg.header_file(), args, "ensnare_headers.hpp",
                                                   "ensnare");
}

fn post_process(Context& ctx) {
   SymGenerator gensym;
   for (auto& type_decl : ctx.type_decls()) {
      for (auto& gensym_type : ctx.cfg.gensym_types()) {
         if (name(type_decl).latest() == gensym_type) {
            name(type_decl).update(name(type_decl).latest() + gensym(16));
         }
      }
   }
}

/// Entrypoint to the c++ part of ensnare.
void run(int argc, const char* argv[]) {
   const Config cfg(argc, argv);
   auto translation_unit = parse_translation_unit(cfg);
   Context ctx(cfg, translation_unit->getASTContext());
   if (visit(*translation_unit, ctx, base_wrap)) {
      post_process(ctx);
      Str output = "import ensnare/runtime\nexport runtime\n";
      output += render(ctx.type_decls());
      output += render(ctx.routine_decls());
      output += render(ctx.variable_decls());
      os::write_file(os::set_file_ext(cfg.output(), ".nim"), output);
   } else {
      fatal("failed to execute visitor");
   }
}
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

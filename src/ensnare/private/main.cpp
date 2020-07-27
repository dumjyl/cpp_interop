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

// FIXME: move to os utils.
#ifdef __ARM_ARCH_ISA_A64
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#elif
#include <filesystem>
namespace fs = std::filesystem;
#endif

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
class HeaderCanonicalizer {
   const clang::SourceManager& source_manager;
   const Vec<Str> headers;

   using IncludeDirMap = llvm::StringMap<Str>;
   IncludeDirMap include_dir_map;

   fn init_include_dir_map(const Config& cfg) const->IncludeDirMap {
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

   fn init_headers(const Config& cfg) const->Vec<Str> {
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

   pub fn operator[](const clang::SourceLocation& loc)->Opt<Str> {
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

   pub fn type_decls() const->const Vec<Node<TypeDecl>>& { return _type_decls; }
   pub fn routine_decls() const->const Vec<Node<RoutineDecl>>& { return _routine_decls; }
   pub fn variable_decls() const->const Vec<Node<VariableDecl>>& { return _variable_decls; }

   priv Vec<const clang::NamedDecl*> decl_stack; ///< To give anonymous tags useful names, we track
                                                 ///< the declaration they were referenced from.
   pub const Config& cfg;

   pub Context(const Config& cfg, const clang::ASTContext& ast_ctx)
      : cfg(cfg), ast_ctx(ast_ctx), header_canonicalizer(cfg, ast_ctx.getSourceManager()) {}

   pub fn decl(int i) const->const clang::NamedDecl& {
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

   fn canon_lookup_decl(const clang::Decl& decl) const-> const clang::Decl* {
      if (auto tag_decl = llvm::dyn_cast<clang::TagDecl>(&decl)) {
         return tag_decl;
      } else {
         return &decl;
      }
   }

   /// Lookup the Node<Type> of a declaration.
   pub fn lookup(const clang::Decl& decl) const->Opt<Node<Type>> {
      auto decl_type = type_lookup.find(canon_lookup_decl(decl));
      if (decl_type == type_lookup.end()) {
         return {};
      } else {
         return decl_type->second;
      }
   }

   /// Record the Node<Type> of a declaration.
   pub fn associate(const clang::Decl& decl, const Node<Type> type) {
      auto maybe_type = lookup(decl);
      if (maybe_type) {
         print(render(decl));
         fatal("unreachable: duplicate type in lookup");
      } else {
         type_lookup[&decl] = type;
      }
   }

   /// Add a type declaration to be rendered.
   pub fn add(const Node<TypeDecl> decl) { _type_decls.push_back(decl); }

   /// Add a routine declaration to be rendered.
   pub fn add(const Node<RoutineDecl> decl) { _routine_decls.push_back(decl); }

   /// Add a variable declaration to be rendered.
   pub fn add(const Node<VariableDecl> decl) { _variable_decls.push_back(decl); }

   /// Filters access to protected and private members.
   pub fn access_guard(const clang::Decl& decl) const->bool {
      return decl.getAccess() == clang::AS_public || decl.getAccess() == clang::AS_none;
   }

   pub fn maybe_header(const clang::NamedDecl& decl)->Opt<Str> {
      return header_canonicalizer[decl.getLocation()];
   }

   pub fn header(const clang::NamedDecl& decl)->Str {
      auto h = maybe_header(decl);
      if (h) {
         return *h;
      } else {
         print(render(decl));
         fatal("failed to canonicalize source location");
      }
   }
};

fn force_wrap(Context& ctx, const clang::NamedDecl& decl)->void;

fn map(Context& ctx, const clang::Type& decl)->Node<Type>;

/// Generic map for pointers to error on null.
template <typename T> fn map(Context& ctx, const T* entity)->Node<Type> {
   if (entity) {
      return map(ctx, *entity);
   } else {
      fatal("tried to map a null entity");
   }
}

/// Maps to an atomic type of some kind. Many of these are obscure and unsupported.
fn map(Context& ctx, const clang::BuiltinType& entity)->Node<Type> {
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
         fatal("this should be unreachable because nim compiles with unsigned "
               "chars");

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
      case clang::BuiltinType::Kind::WChar_U: // 'wchar_t' for targets where it's unsigned
      case clang::BuiltinType::Kind::WChar_S: // 'wchar_t' for targets where it's signed
                                              // FIXME: do we care about the difference?
         return builtins::_wchar;
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
      case clang::BuiltinType::Kind::Half:
         // FIXME: return ctx.builtins._ocl_float16;
         return builtins::_neon_float16;
      case clang::BuiltinType::Kind::Float16: return builtins::_float16;
      case clang::BuiltinType::Kind::Float128: return builtins::_float128;
      case clang::BuiltinType::Kind::NullPtr: return builtins::_nullptr;
      default:
         entity.dump();
         fatal("unreachable: builtin type: ",
               entity.getNameAsCString(clang::PrintingPolicy(clang::LangOptions())));
   }
} // namespace ensnare::ct

/// Map a const/volatile/__restrict qualified type.

fn map(Context& ctx, const clang::QualType& entity)->Node<Type> {
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

/// tldr: map_wrap enforces wrapping types decls that we need while wrapping as little as possible.
///
/// Problem:
/// Consider the function we would like to wrap: `int foo(SomeClass blah)`.
/// `SomeClass` exists within a header we were not tasked to wrap, so we didn't.
/// But we need to produce a type declaration for it regardless, to not render our wrapper useless
/// with an undeclared identifier error. We can't simply call wrap, as that would fail for
/// the same reasons we hadn't wrapped it yet already.
///
/// Solution:
/// map_wrap is our way of enforcing wrapping when we need to reference a type.
/// When we encouter a type like `SomeClass` we increment a counter and call wrap and decrement when
/// we return. wrap shall check this counter at strategic points to enforce wrapping.
///
/// New Problem:
/// When we wrap a declaration we also wrap any associated behavior and data. Those decls may
/// reference more types like `SomeClass`. We end up binding a whole lot more than the user wanted.
///
/// Solution: map_wrap mode also prevents binding associated declarations.
///
/// FIXME: consider if it is cleaner to have map_wrap a separate set of functions sharing helpers.
/// FIXME: move this documentation

/// This maps certain kinds of declarations by first trying to look them up in the context.
/// If it is not in the context, we enforce wrapping it and return the mapped result.
fn map(Context& ctx, const clang::NamedDecl& decl)->Node<Type> {
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

using TypeStack = Vec<std::reference_wrapper<const clang::Type>>;

fn map(Context& ctx, const clang::Type& type)->Node<Type> {
   switch (type.getTypeClass()) {
      case clang::Type::TypeClass::Elaborated:
         // Map an elaborated type. An elaborated type is a syntactic node to not lose information
         // in the clang AST. We care about semantics so we can ignore this.
         return map(ctx, llvm::cast<clang::ElaboratedType>(type).getNamedType());
      case clang::Type::TypeClass::Typedef:
         return map(ctx, llvm::cast<clang::TypedefType>(type).getDecl());
      case clang::Type::TypeClass::Record:
         return map(ctx, get_definition(llvm::cast<clang::RecordType>(type).getDecl()));
      case clang::Type::TypeClass::Enum:
         return map(ctx, get_definition(llvm::cast<clang::EnumType>(type).getDecl()));
      case clang::Type::TypeClass::Pointer: {
         const auto& ty = llvm::cast<clang::PointerType>(type);
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
} // namespace ensnare

fn transfer(Context& ctx, const clang::ParmVarDecl& param)->ParamDecl {
   return ParamDecl(param.getNameAsString(), map(ctx, param.getType()));
}

fn transfer(Context& ctx, const clang::FunctionDecl& decl)->Vec<ParamDecl> {
   Vec<ParamDecl> result;
   for (auto param : decl.parameters()) {
      result.push_back(transfer(ctx, *param));
   }
   return result;
}

/// Map the return type if it has one.
fn transfer_return_type(Context& ctx, const clang::FunctionDecl& decl)->Opt<Node<Type>> {
   auto result = decl.getReturnType();
   if (result->isVoidType()) {
      return {};
   } else {
      return map(ctx, result);
   }
}

fn wrap_non_template(Context& ctx, const clang::FunctionDecl& decl) {
   ctx.add(node<RoutineDecl>(FunctionDecl(decl.getNameAsString(), decl.getQualifiedNameAsString(),
                                          ctx.header(decl), transfer(ctx, decl),
                                          transfer_return_type(ctx, decl))));
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
      ctx.add(node<RoutineDecl>(ConstructorDecl(decl.getQualifiedNameAsString(), ctx.header(decl),
                                                map(ctx, decl.getParent()), transfer(ctx, decl))));
   }
}

fn wrap(Context& ctx, const clang::CXXConstructorDecl& decl) {
   assert_non_template(decl);
   wrap_non_template(ctx, decl);
}

fn wrap_non_template(Context& ctx, const clang::CXXMethodDecl& decl) {
   if (ctx.access_guard(decl)) {
      ctx.add(node<RoutineDecl>(MethodDecl(decl.getNameAsString(), decl.getQualifiedNameAsString(),
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
fn qualified_nim_name(Context& ctx, const clang::NamedDecl& decl)->Str {
   return replace(decl.getQualifiedNameAsString(), "::", "-");
}

fn transfer(Context& ctx, const clang::CXXRecordDecl::field_range fields)->Vec<RecordFieldDecl> {
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

fn transfer(Context& ctx, const clang::EnumDecl::enumerator_range fields)->Vec<EnumFieldDecl> {
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

fn transfer(Context& ctx, const clang::NamedDecl& name_decl, const clang::EnumDecl& decl)
    ->Node<TypeDecl> {
   return node<TypeDecl>(EnumTypeDecl(qualified_nim_name(ctx, name_decl),
                                      name_decl.getQualifiedNameAsString(), ctx.header(decl),
                                      transfer(ctx, decl.enumerators())));
}

fn wrap_enum(Context& ctx, const clang::EnumDecl& decl) {
   if (ctx.access_guard(decl) && has_name(decl)) {
      auto type_decl = transfer(ctx, decl, decl);
      ctx.associate(decl, node<Type>(deref<EnumTypeDecl>(type_decl).name));
      ctx.add(type_decl);
   }
}

fn record_name(Context& ctx, const clang::NamedDecl& decl)->Node<Sym> {
   return has_name(decl) ? node<Sym>(qualified_nim_name(ctx, decl))
                         : node<Sym>("type_of(" + qualified_nim_name(ctx, ctx.decl(1)) + ")");
}

/// FIXME: document this.
fn transfer(Context& ctx, const Node<Sym> sym, const clang::NamedDecl& name_decl,
            const clang::CXXRecordDecl& decl)
    ->Node<TypeDecl> {
   if (has_name(name_decl)) {
      return node<TypeDecl>(RecordTypeDecl(sym, name_decl.getQualifiedNameAsString(),
                                           ctx.header(name_decl), transfer(ctx, decl.fields())));
   } else {
      return node<TypeDecl>(
          RecordTypeDecl(sym, "decltype(" + ctx.decl(1).getQualifiedNameAsString() + ")",
                         ctx.header(name_decl), transfer(ctx, decl.fields())));
   }
}

fn register_record_name(Context& ctx, const clang::CXXRecordDecl& decl)->Node<Sym> {
   auto result = record_name(ctx, decl);
   ctx.associate(decl, node<Type>(result));
   return result;
}

/// A definition is expected.
fn wrap_record(Context& ctx, const clang::CXXRecordDecl& decl) {
   log("record", decl);
   // Unnamed types are wrapped when they get used.
   if (has_name(decl)) {
      auto name = register_record_name(ctx, decl);
      auto type_decl = transfer(ctx, name, decl, decl);
      ctx.add(type_decl);
      wrap_methods(ctx, decl.methods());
   }
}

/// A definition is expected.
fn force_wrap_record(Context& ctx, const clang::CXXRecordDecl& decl) {
   log("force record", decl);
   register_record_name(ctx, decl);
   auto name = record_name(ctx, decl);
   auto type_decl = transfer(ctx, name, decl, decl);
   ctx.add(type_decl);
}

/// Check if `decl` is from <cstddef>.
fn get_cstddef_item(Context& ctx, const clang::NamedDecl& decl)->Opt<Node<Type>> {
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
   auto name = node<Sym>(qualified_nim_name(ctx, decl));
   ctx.associate(decl, node<Type>(name));
   auto type_decl = node<TypeDecl>(AliasTypeDecl(name, map(ctx, decl.getUnderlyingType())));
   ctx.add(type_decl);
}

fn wrap_anon_record_typedef(Context& ctx, const clang::TypedefDecl& typedef_decl,
                            const clang::CXXRecordDecl& decl) {
   auto name = record_name(ctx, typedef_decl);
   auto type = node<Type>(name);
   ctx.associate(decl, type);
   ctx.associate(typedef_decl, type);
   auto type_decl = transfer(ctx, name, typedef_decl, decl);
   ctx.add(type_decl);
   wrap_methods(ctx, decl.methods());
}

fn wrap_anon_enum_typedef(Context& ctx, const clang::TypedefDecl& typedef_decl,
                          const clang::EnumDecl& decl) {
   auto type_decl = transfer(ctx, typedef_decl, decl);
   auto type = node<Type>(deref<EnumTypeDecl>(type_decl).name);
   ctx.associate(decl, type);
   ctx.associate(typedef_decl, type);
   ctx.add(type_decl);
}

fn wrap_anon_typedef(Context& ctx, const clang::TypedefDecl& decl, const clang::TagDecl& tag_decl) {
   if (llvm::isa<clang::CXXRecordDecl>(tag_decl)) {
      wrap_anon_record_typedef(ctx, decl, llvm::cast<clang::CXXRecordDecl>(tag_decl));
   } else if (llvm::isa<clang::EnumDecl>(tag_decl)) {
      wrap_anon_enum_typedef(ctx, decl, llvm::cast<clang::EnumDecl>(tag_decl));
   } else {
      fatal("unhandled: anon tag typedef");
   }
}

/// Oof. There are a lot of interacting features here. Among them: typedefs, enums, records,
/// anonymity, stripping _t suffix.
fn wrap_typedef(Context& ctx, const clang::TypedefDecl& decl) {
   // We map certain typedefs from the stdlib to builtins.
   // FIXME: This tag special handling is only needed for `typedef` not `using`.
   if (const auto maybe_tag_decl = owns_tag(decl)) {
      log("tag", decl);
      const clang::TagDecl& tag_decl = *maybe_tag_decl;
      if (has_name(tag_decl)) {
         if (eq_ident(decl, tag_decl)) { // The tag decl has already been visited.
                                         // We only need to make the typedef decl's type map to
                                         // the tag's type.
            ctx.associate(decl, map(ctx, tag_decl));
         } else if (ctx.cfg.fold_type_suffix() &&
                    decl.getQualifiedNameAsString() == tag_decl.getQualifiedNameAsString() + "_t") {
            auto type = map(ctx, tag_decl);
            if (is<Node<Sym>>(type)) {
               auto name = deref<Node<Sym>>(type);
               name->update(name->latest() + "_t");
            } else {
               fatal("unreachable: fold type suffix");
            }
            ctx.associate(decl, type);
         } else {
            wrap_simple_alias(ctx, decl);
         }
      } else { // The typedef will consume the anonymous inner tag decl.
         wrap_anon_typedef(ctx, decl, tag_decl);
      }
   } else if (const auto cstddef_item = get_cstddef_item(ctx, decl)) {
      // References to the typedef should fold to our builtins.
      ctx.associate(decl, *cstddef_item);
   } else {
      log("alias", decl);
      wrap_simple_alias(ctx, decl);
   }
}

fn force_wrap(Context& ctx, const clang::NamedDecl& named_decl)->void {
   if (!ctx.lookup(named_decl)) {
      ctx.push(named_decl);
      switch (named_decl.getKind()) {
         case clang::Decl::Kind::CXXRecord:
            force_wrap_record(ctx, get_definition(llvm::cast<clang::CXXRecordDecl>(named_decl)));
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

fn wrap(Context& ctx, const clang::NamedDecl& named_decl)->void {
   if (!ctx.lookup(named_decl) && ctx.maybe_header(named_decl)) {
      ctx.push(named_decl);
      switch (named_decl.getKind()) {
         case clang::Decl::Kind::TypeAlias:
            wrap_simple_alias(ctx, llvm::cast<clang::TypeAliasDecl>(named_decl));
            break;
         case clang::Decl::Kind::Typedef:
            wrap_typedef(ctx, llvm::cast<clang::TypedefDecl>(named_decl));
            break;
         case clang::Decl::Kind::CXXRecord:
            wrap_record(ctx, get_definition(llvm::cast<clang::CXXRecordDecl>(named_decl)));
            break;
         case clang::Decl::Kind::Enum:
            wrap_enum(ctx, get_definition(llvm::cast<clang::EnumDecl>(named_decl)));
            break;
         case clang::Decl::Kind::Function:
            wrap_function(ctx, llvm::cast<clang::FunctionDecl>(named_decl));
            break;
         case clang::Decl::Kind::Var: {
            auto& decl = llvm::cast<clang::VarDecl>(named_decl);
            if (ctx.access_guard(decl) && decl.hasGlobalStorage() && !decl.isStaticLocal()) {
               ctx.add(node<VariableDecl>(qualified_nim_name(ctx, decl),
                                          decl.getQualifiedNameAsString(), ctx.header(decl),
                                          map(ctx, decl.getType())));
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

fn prefixed_search_paths()->Vec<Str> {
   Vec<Str> result;
   for (const auto& search_path : Header::search_paths()) {
      result.push_back("-isystem" + search_path);
   }
   return result;
}

/// Load a translation unit from user provided arguments with additional include path arguments.
fn parse_translation_unit(const Config& cfg)->std::unique_ptr<clang::ASTUnit> {
   Vec<Str> args = {"-xc++"};
   auto search_paths = prefixed_search_paths();
   args.insert(args.end(), search_paths.begin(), search_paths.end());
   // The users args get placed after for higher priority.
   args.insert(args.end(), cfg.user_clang_args().begin(), cfg.user_clang_args().end());
   return clang::tooling::buildASTFromCodeWithArgs(cfg.header_file(), args, "ensnare_headers.hpp",
                                                   "ensnare");
}

/// Entrypoint to the c++ part of ensnare.
void run(int argc, const char* argv[]) {
   const Config cfg(argc, argv);
   auto translation_unit = parse_translation_unit(cfg);
   Context ctx(cfg, translation_unit->getASTContext());
   if (visit(*translation_unit, ctx, base_wrap)) {
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

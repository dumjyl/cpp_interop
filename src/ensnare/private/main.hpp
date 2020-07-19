// FIXME: typedef handling with

#pragma once

#include "ensnare/private/bit_utils.hpp"
#include "ensnare/private/config.hpp"
#include "ensnare/private/headers.hpp"
#include "ensnare/private/ir.hpp"
#include "ensnare/private/os_utils.hpp"
#include "ensnare/private/rendering.hpp"
#include "ensnare/private/runtime.hpp"
#include "ensnare/private/str_utils.hpp"
#include "ensnare/private/utils.hpp"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/Tooling.h"

// preserve order
#include "ensnare/private/syn.hpp"

namespace ensnare {

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
   priv std::size_t alt_counter;
   pub Context(const Config& cfg, const clang::ASTContext& ast_ctx)
      : cfg(cfg), ast_ctx(ast_ctx), alt_counter(0) {}

   pub fn inc_alt() { alt_counter += 1; }

   pub fn dec_alt() { alt_counter -= 1; }

   pub fn alt() -> bool { return alt_counter != 0; }

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

fn wrap(Context& ctx, const clang::NamedDecl& decl) -> void;

fn map(Context& ctx, const clang::Type& decl) -> Node<Type>;

template <typename T> fn map(Context& ctx, const T* entity) {
   if (entity) {
      return map(ctx, *entity);
   } else {
      fatal("tried to map a null entity");
   }
}

// Maps to an atomic type of some kind. Many of these are obscure and
// unsupported.
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
      default:
         entity.dump();
         fatal("unreachable: builtin type: ",
               entity.getNameAsCString(clang::PrintingPolicy(clang::LangOptions())));
   }
} // namespace ensnare::ct

fn map(Context& ctx, const clang::QualType& entity) -> Node<Type> {
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

fn map(Context& ctx, const Node<TypeDecl>& entity) -> Node<Type> {
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

fn map(Context& ctx, const clang::ElaboratedType& entity) -> Node<Type> {
   return map(ctx, entity.getNamedType());
}

fn alt_wrap(Context& ctx, const clang::NamedDecl& decl) {
   ctx.inc_alt();
   wrap(ctx, decl);
   ctx.dec_alt();
}

fn map(Context& ctx, const clang::NamedDecl& decl) -> Node<Type> {
   auto type = ctx.lookup(decl);
   if (type) {
      return *type;
   } else {
      alt_wrap(ctx, decl);
      auto type = ctx.lookup(decl);
      if (type) {
         return *type;
      } else {
         decl.dump();
         fatal("failed to map type");
      }
   }
}

// FIXME: add the restrict_wrap counter.

// If a `Type` has a `Decl` representation, always map that. It seems to work.
fn map(Context& ctx, const clang::TypedefType& type) -> Node<Type> {
   return map(ctx, type.getDecl());
}

fn map(Context& ctx, const clang::RecordType& type) -> Node<Type> { return map(ctx, type.getDecl()); }

fn map(Context& ctx, const clang::EnumType& type) -> Node<Type> { return map(ctx, type.getDecl()); }

fn map(Context& ctx, const clang::PointerType& type) -> Node<Type> {
   return node<Type>(PtrType(map(ctx, type.getPointeeType())));
}

fn map(Context& ctx, const clang::ReferenceType& type) -> Node<Type> {
   return node<Type>(RefType(map(ctx, type.getPointeeType())));
}

fn map(Context& ctx, const clang::VectorType& type) -> Node<Type> {
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

fn map(Context& ctx, const clang::ParenType& type) -> Node<Type> {
   return map(ctx, type.getInnerType());
}

fn map(Context& ctx, const clang::DecltypeType& type) -> Node<Type> {
   return map(ctx, type.getUnderlyingType());
}

fn map(Context& ctx, const clang::EnumDecl::enumerator_range fields) -> Vec<EnumFieldDecl> {
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

#define DISPATCH(kind)                                                                             \
   case clang::Type::TypeClass::kind: {                                                            \
      return map(ctx, llvm::cast<clang::kind##Type>(entity));                                      \
   }

fn map(Context& ctx, const clang::Type& entity) -> Node<Type> {
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

fn decl_contexts(const clang::NamedDecl& decl) -> llvm::SmallVector<const clang::DeclContext*, 8> {
   llvm::SmallVector<const clang::DeclContext*, 8> result;
   const clang::DeclContext* ctx = decl.getDeclContext();
   while (ctx) {
      if (llvm::isa<clang::NamedDecl>(ctx)) {
         result.push_back(ctx);
      }
      ctx = ctx->getParent();
   }
   return result;
}

fn has_name(const clang::TagDecl& decl) -> bool { return decl.getIdentifier(); }

fn qualified_nim_name(Context& ctx, const clang::NamedDecl& decl) -> Str {
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

fn wrap(Context& ctx, const clang::RecordDecl& decl, bool force_wrap = false) {
   if (has_name(decl) || force_wrap) {
      RecordTypeDecl record(qualified_nim_name(ctx, decl), decl.getQualifiedNameAsString(),
                            expect(ctx.header(decl), "failed to canonicalize source location"));
      auto type_decl = node<TypeDecl>(record);
      ctx.set(decl, map(ctx, type_decl));
      ctx.add(type_decl);
   }
}

fn wrap(Context& ctx, const clang::CXXRecordDecl::method_range methods) {
   for (auto method : methods) {
      if (method) {
         wrap(ctx, *method);
      } else {
         fatal("unreachable: wrap method");
      }
   }
}

fn wrap(Context& ctx, const clang::CXXRecordDecl& decl) {
   if (has_name(decl)) {
      wrap(ctx, static_cast<const clang::RecordDecl&>(decl));
      wrap(ctx, decl.methods());
   }
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

fn wrap_alias(Context& ctx, const clang::TypedefNameDecl& decl) {
   auto qualified_name = decl.getQualifiedNameAsString();
   AliasTypeDecl alias(qualified_nim_name(ctx, decl), map(ctx, decl.getUnderlyingType()));
   auto type_decl = node<TypeDecl>(alias);
   ctx.set(decl, map(ctx, type_decl));
   ctx.add(type_decl);
}

// This means it names an unnamed type
fn defines_type(const clang::TypedefNameDecl& decl) -> bool {
   const auto tag = llvm::dyn_cast<clang::TagType>(decl.getUnderlyingType());
   return tag && !has_name(*tag->getDecl());
}

fn wrap(Context& ctx, const clang::TypedefNameDecl& decl) {
   // This could be c++ `TypeAliasDecl` or a `TypedefDecl`.
   // Either way, we produce `type AliasName* = UnderlyingType`

   // We map certain typedefs from the stdlib to builtins.
   auto cstddef_item = get_cstddef_item(ctx, decl);
   if (cstddef_item) {
      ctx.set(decl, *cstddef_item);
   } else if (defines_type(decl)) {
      // The inner type is an unnamed tag type. It has not been wrapped yet.
      // FIXME
      if (const auto record_decl = llvm::dyn_cast<clang::CXXRecordDecl>(tag_decl)) {

      } else {
         fatal("unreachable: anon tag typedef");
      }
   } else {
      wrap_alias(ctx, decl);
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

fn wrap_return_type(Context& ctx, const clang::FunctionDecl& decl) -> Opt<Node<Type>> {
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
                    wrap_formals(ctx, decl), wrap_return_type(ctx, decl))));
}

fn is_visible(const clang::VarDecl& decl) -> bool {
   return decl.hasGlobalStorage() && !decl.isStaticLocal();
}

fn wrap(Context& ctx, const clang::VarDecl& decl) {
   if (ctx.access_filter(decl) && is_visible(decl)) {
      ctx.add(node<VariableDecl>(qualified_nim_name(ctx, decl), decl.getQualifiedNameAsString(),
                                 expect(ctx.header(decl), "failed to canonicalize source location"),
                                 map(ctx, decl.getType())));
   }
}

fn wrap(Context& ctx, const clang::FunctionDecl& decl) {
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

fn wrap(Context& ctx, const clang::CXXConstructorDecl& decl) {
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
          map(ctx, decl.getParent()), wrap_formals(ctx, decl), wrap_return_type(ctx, decl))));
   }
}

fn wrap(Context& ctx, const clang::CXXMethodDecl& decl) {
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

fn wrap(Context& ctx, const clang::EnumDecl& decl) {
   if (ctx.access_filter(decl)) {
      ctx.add(node<TypeDecl>(
          EnumTypeDecl(qualified_nim_name(ctx, decl), decl.getQualifiedNameAsString(),
                       expect(ctx.header(decl), "failed to canonicalize source location"),
                       map(ctx, decl.enumerators()))));
   }
}

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

fn wrap(Context& ctx, const clang::NamedDecl& decl) -> void {
   auto header = ctx.header(decl);
   if (header) {
      switch (decl.getKind()) {
         // DISCARD(Record); // We should not get these here.
         DISCARD(Namespace); // FIXME: doing something with this would be good.
         DISCARD(NamespaceAlias);
         DISCARD(Field); // handled in RecordDecl
         DISCARD(ParmVar);
         DISCARD(Using); // not relevant

         DISCARD(EnumConstant); // handled in EnumDecl

         DISCARD(CXXMethod);      // handled in CXXRecordDecl
         DISCARD(CXXConstructor); // handled in CXXRecordDecl
         DISCARD(CXXDestructor);  // handled in CXXRecordDecl

         DISPATCH(TypeAlias);
         DISPATCH(Typedef);
         DISPATCH(CXXRecord);
         DISPATCH(Enum);
         DISPATCH(Function); // A free function
         DISPATCH(Var);
         default:
            decl.dump();
            fatal("unhandled wrapping: ", decl.getDeclKindName(), "Decl");
      }
   }
}

fn wrap(Context& ctx, const clang::Decl& decl) {
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
      // post NamedDecls
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
      output += render(ctx.type_decls());
      output += render(ctx.routine_decls());
      output += render(ctx.variable_decls());
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

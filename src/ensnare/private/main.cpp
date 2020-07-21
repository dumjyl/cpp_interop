#include "ensnare/private/main.hpp"

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

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
///
/// Contains constants to the low level atomic (in the AST sense) types for building more
/// complicated types.
///
namespace builtins {

// FIXME: some of the size comments are innacurate and only reflect the LP64 data model.
//        in the future these constraints should be verified in runtime.hpp.

/// Initalize a builtin type.
fn init(const char* name) -> Node<Type> { return node<Type>(AtomType(name)); }

/// An atomic Type.
using Builtin = const Node<Type>;

/// Represents `signed char`.
Builtin _schar = init("CppSChar");
/// Represent `signed short`.
Builtin _short = init("CppShort");
/// Represents `signed int`.
Builtin _int = init("CppInt");
/// Represents `signed long`.
Builtin _long = init("CppLong");
/// Represents `signed long long`.
Builtin _long_long = init("CppLongLong");
/// Represents `unsigned char`.
Builtin _uchar = init("CppUChar");
/// Represents `unsigned short`.
Builtin _ushort = init("CppUShort");
/// Represents `unsigned int`.
Builtin _uint = init("CppUInt");
/// Represents `unsigned long`.
Builtin _ulong = init("CppULong");
/// Represents `unsigned long long`.
Builtin _ulong_long = init("CppULongLong");
/// Represents `char`. Nim passes `funsigned-char` so this will always be unsigned for us.
Builtin _char = init("CppChar");
/// Represents `whcar_t`.
Builtin _wchar = init("CppWChar");
/// Represents `char8_t`.
Builtin _char8 = init("CppChar8");
/// Represents `char16_t`.
Builtin _char16 = init("CppChar16");
/// Represents `char32_t`.
Builtin _char32 = init("CppChar32");
/// Represents `bool`.
Builtin _bool = init("CppBool");
/// Represents `float`.
Builtin _float = init("CppFloat");
/// Represents `double`.
Builtin _double = init("CppDouble");
/// Represents `long double`.
Builtin _long_double = init("CppLongDouble");
// Represents `void`
Builtin _void = init("CppVoid");
/// Represents `__int128_t`
Builtin _int128 = init("CppInt128");
/// Represents `__uint128_t`
Builtin _uint128 = init("CppUInt128");
/// Represents `__fp16`
Builtin _neon_float16 = init("CppNeonFloat16");
/// Represents `half`
Builtin _ocl_float16 = init("CppOCLFloat16");
/// Represents `_Float16`
Builtin _float16 = init("CppFloat16");
/// Represents `__float128`
Builtin _float128 = init("CppFloat128");
// we treat cstddef as builtins too. But why?
/// Represents `std::size_t`
Builtin _size = init("CppSize");
/// Represents `std::ptrdiff_t`.
Builtin _ptrdiff = init("CppPtrDiff");
/// Represents `std::max_align_t`.
Builtin _max_align = init("CppMaxAlign");
/// Represents `std::byte`.
Builtin _byte = init("CppByte");
/// Represents `decltype(nullptr)` and `std::nullptr_t`.
Builtin _nullptr = init("CppNullPtr");
} // namespace builtins

/// Holds ensnare's state.
///
/// A `Context` must not outlive a `Config`
class Context {
   pub const clang::ASTContext& ast_ctx;
   priv Map<const clang::Decl*, Node<Type>>
       type_lookup; ///< For mapping bound declarations to exisiting types.
                    ///<
   READ_ONLY(Vec<Node<TypeDecl>>, type_decls);         // Types to output.
   READ_ONLY(Vec<Node<RoutineDecl>>, routine_decls);   // Functions to output.
   READ_ONLY(Vec<Node<VariableDecl>>, variable_decls); // Variables to output.

   pub const Config& cfg;
   priv std::size_t alt_counter;
   pub Context(const Config& cfg, const clang::ASTContext& ast_ctx)
      : cfg(cfg), ast_ctx(ast_ctx), alt_counter(0) {}

   /// Are we in the alternative binding mode.
   /// See alt_wrap for more information
   pub fn alt() -> bool { return alt_counter != 0; }
   pub fn inc_alt() { alt_counter += 1; }
   pub fn dec_alt() { alt_counter -= 1; }

   /// Get the filename from a source location.
   pub fn filename(const clang::SourceLocation& loc) const -> llvm::StringRef {
      return ast_ctx.getSourceManager().getFilename(loc);
   }

   /// Lookup the Node<Type> of a declaration.
   pub inline fn lookup(const clang::Decl& decl) const -> Opt<Node<Type>> {
      auto decl_type = type_lookup.find(&decl);
      if (decl_type == type_lookup.end()) {
         // print("cache miss: ", llvm::cast<clang::NamedDecl>(decl).getQualifiedNameAsString());
         return {};
      } else {
         // print("cache hit: ", llvm::cast<clang::NamedDecl>(decl).getQualifiedNameAsString());
         return decl_type->second;
      }
   }

   /// Record the Node<Type> of a declaration.
   pub fn associate(const clang::Decl& decl, const Node<Type> type) {
      auto maybe_type = lookup(decl);
      if (maybe_type) {
         fatal("unreachable: duplicate type in lookup");
      } else {
         // print("associate: ", llvm::cast<clang::NamedDecl>(decl).getQualifiedNameAsString());
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
   pub fn access_guard(const clang::Decl& decl) const -> bool {
      return decl.getAccess() == clang::AS_public || decl.getAccess() == clang::AS_none;
   }

   /// Find the user passed header (if any) that `decl` is declared within.
   pub fn header(const clang::NamedDecl& decl) const -> Opt<Str> {
      for (auto const& header : cfg.headers()) {
         if (ends_with(filename(decl.getLocation()), header)) {
            return header;
         }
      }
      return {};
   }
};

fn wrap(Context& ctx, const clang::NamedDecl& decl) -> void;

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
      case clang::BuiltinType::Kind::OCLReserveID: {
         fatal("opencl builtins unsupported");
      }
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
      case clang::BuiltinType::Kind::SChar:
         return builtins::_schar;
      case clang::BuiltinType::Kind::Short:
         return builtins::_short;
      case clang::BuiltinType::Kind::Int:
         return builtins::_int;
      case clang::BuiltinType::Kind::Long:
         return builtins::_long;
      case clang::BuiltinType::Kind::LongLong:
         return builtins::_long_long;
      case clang::BuiltinType::Kind::UChar:
         return builtins::_char;
      case clang::BuiltinType::Kind::UShort:
         return builtins::_ushort;
      case clang::BuiltinType::Kind::UInt:
         return builtins::_uint;
      case clang::BuiltinType::Kind::ULong:
         return builtins::_ulong;
      case clang::BuiltinType::Kind::ULongLong:
         return builtins::_ulong_long;
      case clang::BuiltinType::Kind::Char_U:
         return builtins::_char;
      case clang::BuiltinType::Kind::WChar_U: // 'wchar_t' for targets where it's unsigned
      case clang::BuiltinType::Kind::WChar_S: // 'wchar_t' for targets where it's signed
                                              // FIXME: do we care about the difference?
         return builtins::_wchar;
      case clang::BuiltinType::Kind::Char8:
         return builtins::_char8;
      case clang::BuiltinType::Kind::Char16:
         return builtins::_char16;
      case clang::BuiltinType::Kind::Char32:
         return builtins::_char32;
      case clang::BuiltinType::Kind::Bool:
         return builtins::_bool;
      case clang::BuiltinType::Kind::Float:
         return builtins::_float;
      case clang::BuiltinType::Kind::Double:
         return builtins::_double;
      case clang::BuiltinType::Kind::LongDouble:
         return builtins::_long_double;
      case clang::BuiltinType::Kind::Void:
         return builtins::_void;
      case clang::BuiltinType::Kind::Int128:
         return builtins::_int128;
      case clang::BuiltinType::Kind::UInt128:
         return builtins::_uint128;
      case clang::BuiltinType::Kind::Half:
         // FIXME: return ctx.builtins._ocl_float16;
         return builtins::_neon_float16;
      case clang::BuiltinType::Kind::Float16:
         return builtins::_float16;
      case clang::BuiltinType::Kind::Float128:
         return builtins::_float128;
      case clang::BuiltinType::Kind::NullPtr:
         return builtins::_nullptr;
      default:
         entity.dump();
         fatal("unreachable: builtin type: ",
               entity.getNameAsCString(clang::PrintingPolicy(clang::LangOptions())));
   }
} // namespace ensnare::ct

/// Map a const/volatile/__restrict qualified type.
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

/// Map a Node<TypeDecl>. These get stored on the right hand side of Context::type_lookup.
/// See Context::associate for more information.
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

/// Map an elaborated type. An elaborated type some is some sugar to not lose information in the
/// clang AST. We care about semantics so it should not matter.
fn map(Context& ctx, const clang::ElaboratedType& entity) -> Node<Type> {
   return map(ctx, entity.getNamedType());
}

/// Wrap a declaration in alternative mode.
fn alt_wrap(Context& ctx, const clang::NamedDecl& decl) {
   ctx.inc_alt();
   wrap(ctx, decl);
   ctx.dec_alt();
}

/// This maps certain kinds of declarations by first trying to look them up in the context.
/// If it is not in the context, we wrap it ourselves and return the mapped result.
///
/// A decl would not be in the context because when we visited it we were not interested in
/// wrapping it.
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
         fatal("unreachable: failed to map type");
      }
   }
}

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
}

fn map(Context& ctx, const clang::ParenType& type) -> Node<Type> {
   return map(ctx, type.getInnerType());
}

fn map(Context& ctx, const clang::DecltypeType& type) -> Node<Type> {
   return map(ctx, type.getUnderlyingType());
}

fn transfer(Context& ctx, const clang::EnumDecl::enumerator_range fields) -> Vec<EnumFieldDecl> {
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

/// Collect the relevant named declaration contexts for a declaration.
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

/// Is this tag declaration declared without an identifier.
fn has_name(const clang::TagDecl& decl) -> bool { return decl.getIdentifier(); }

/// A qualified name contains all opaque namespace and type contexts.
/// Instead of the c++ double colon we use a minus and stropping.
fn qualified_nim_name(Context& ctx, const clang::NamedDecl& decl) -> Str {
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

fn transfer(Context& ctx, const clang::NamedDecl& name_decl, const clang::RecordDecl& decl)
    ->Node<TypeDecl> {
   return node<TypeDecl>(
       RecordTypeDecl(qualified_nim_name(ctx, name_decl), name_decl.getQualifiedNameAsString(),
                      expect(ctx.header(name_decl), "failed to canonicalize source location")));
}

fn transfer(Context& ctx, const clang::RecordDecl& decl) -> Node<TypeDecl> {
   return transfer(ctx, decl, decl);
}

fn should_wrap_record(const Context& ctx, const clang::RecordDecl& decl) -> bool {
   // Unnamed types are wrapped when they get used.
   return has_name(decl);
}

fn wrap(Context& ctx, const clang::RecordDecl& decl) {
   if (should_wrap_record(ctx, decl)) {
      auto type_decl = transfer(ctx, decl);
      ctx.associate(decl, map(ctx, type_decl));
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
   if (should_wrap_record(ctx, decl)) {
      auto type_decl = transfer(ctx, decl);
      ctx.associate(decl, map(ctx, type_decl));
      ctx.add(type_decl);
      wrap(ctx, decl.methods());
   }
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

/// Get the tag declaration that in defined inline with this typedef.
fn owns_tag(const clang::TypedefNameDecl& decl) -> OptRef<const clang::TagDecl> {
   // FIXME: how does a `typedef struct Foo tag`, where `struct Foo` is already declared.
   auto elaborated = llvm::dyn_cast<clang::ElaboratedType>(decl.getUnderlyingType());
   if (elaborated) {
      auto tag = elaborated->getOwnedTagDecl();
      if (tag) {
         return *tag;
      }
   }
   return {};
}

fn wrap_alias(Context& ctx, const clang::TypedefNameDecl& decl) {
   auto type_decl = node<TypeDecl>(
       AliasTypeDecl(qualified_nim_name(ctx, decl), map(ctx, decl.getUnderlyingType())));
   ctx.associate(decl, map(ctx, type_decl));
   ctx.add(type_decl);
}

// When we wrap an anonymous tag typedef we need both the tag and typedef decls to map to the same
// type. There are probably some bugs lurking there. Worst case we give these a TAG_Foo.

fn wrap_anon(Context& ctx, const clang::TypedefNameDecl& typedef_decl,
             const clang::CXXRecordDecl& decl) {
   auto type_decl = transfer(ctx, typedef_decl, decl);
   auto type = map(ctx, type_decl);
   ctx.associate(decl, type);
   ctx.associate(typedef_decl, type);
   ctx.add(type_decl);
   wrap(ctx, decl.methods());
}

fn wrap_anon(Context& ctx, const clang::TypedefNameDecl& typedef_decl,
             const clang::RecordDecl& decl) {
   auto type_decl = transfer(ctx, typedef_decl, decl);
   auto type = map(ctx, type_decl);
   ctx.associate(decl, type);
   ctx.associate(typedef_decl, type);
   ctx.add(type_decl);
}

fn wrap_anon(Context& ctx, const clang::TypedefNameDecl& typedef_decl,
             const clang::EnumDecl& decl) {
   // auto type_decl = ;
   // ctx.associate(decl, type_decl);
   // ctx.associate(typedef_decl, type_decl);
   // ctx.add(type_decl);
}

///// Does this typedef define an anonymous tag type.
// fn is_definition(const clang::TypedefNameDecl& decl)    ->    bool {
//   auto maybe_tag = tag_decl(decl);
//   if (maybe_tag) {
//      return has_name(*maybe_tag);
//   } else {
//      return false;
//   }
//}

///// Check that we are actually aliasing a type and not exposing something in the tag namespace.
// fn is_distinct(const clang::TypedefNameDecl& decl)    ->    bool {
//   auto maybe_tag = tag_decl(decl);
//   if (maybe_tag) {
//      const clang::TagDecl& tag = *maybe_tag;
//      return tag.getNameAsString() != decl.getNameAsString(); // FIXME: bad
//   } else {
//      return true;
//   }
//}

fn wrap_anon(Context& ctx, const clang::TypedefNameDecl& decl, const clang::TagDecl& tag_decl) {
   if (llvm::isa<clang::CXXRecordDecl>(tag_decl)) {
      wrap_anon(ctx, decl, llvm::cast<clang::CXXRecordDecl>(tag_decl));
   } else if (llvm::isa<clang::RecordDecl>(tag_decl)) {
      wrap_anon(ctx, decl, llvm::cast<clang::RecordDecl>(tag_decl));
   } else if (llvm::isa<clang::EnumDecl>(tag_decl)) {
      wrap_anon(ctx, decl, llvm::cast<clang::RecordDecl>(tag_decl));
   } else {
      fatal("unhandled: anon tag typedef");
   }
}

fn eq_ident(const clang::NamedDecl& a, const clang::NamedDecl& b) -> bool {
   // FIXME: find the right api.
   return a.getQualifiedNameAsString() == b.getQualifiedNameAsString();
}

/// Wrap a type aliasing declaration.
/// We produce something like `type AliasName* = UnderlyingType`.
/// Typedefs for anonymous and tag types with identical names have special handling.
fn wrap(Context& ctx, const clang::TypedefNameDecl& decl) {
   // We map certain typedefs from the stdlib to builtins.
   // print("wrap typedef: ", decl.getQualifiedNameAsString());
   // FIXME: This tag special handling is only needed for `typedef` not `using`.
   if (const auto tag_decl = owns_tag(decl)) {
      if (has_name(*tag_decl)) {
         if (eq_ident(decl, *tag_decl)) {
            ctx.associate(decl, map(ctx, *tag_decl));
         } else {
            wrap_alias(ctx, decl);
         }
      } else {
         wrap_anon(ctx, decl, *tag_decl);
      }
   } else if (const auto cstddef_item = get_cstddef_item(ctx, decl)) {
      // References to the typedef should fold to our builtins.
      ctx.associate(decl, *cstddef_item);
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

/// Map the return type if it has one.
fn wrap_return_type(Context& ctx, const clang::FunctionDecl& decl) -> Opt<Node<Type>> {
   auto result = decl.getReturnType();
   if (result->isVoidType()) {
      return {};
   } else {
      return map(ctx, result);
   }
}

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
   if (ctx.access_guard(decl) && is_visible(decl)) {
      ctx.add(node<VariableDecl>(qualified_nim_name(ctx, decl), decl.getQualifiedNameAsString(),
                                 expect(ctx.header(decl), "failed to canonicalize source location"),
                                 map(ctx, decl.getType())));
   }
}

fn assert_non_template(const clang::FunctionDecl& decl) {
   switch (decl.getTemplatedKind()) {
      case clang::FunctionDecl::TK_NonTemplate:
         break;
      case clang::FunctionDecl::TK_FunctionTemplate:
      case clang::FunctionDecl::TK_MemberSpecialization:
      case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization: {
         fatal("only simple function signatures are supported.");
      }
   }
}

fn wrap(Context& ctx, const clang::FunctionDecl& decl) {
   assert_non_template(decl);
   wrap_non_template(ctx, decl);
}

fn wrap_non_template(Context& ctx, const clang::CXXConstructorDecl& decl) {
   if (ctx.access_guard(decl)) {
      ctx.add(node<RoutineDecl>(
          ConstructorDecl(decl.getQualifiedNameAsString(),
                          expect(ctx.header(decl), "failed to canonicalize source location"),
                          map(ctx, decl.getParent()), wrap_formals(ctx, decl))));
   }
}

fn wrap(Context& ctx, const clang::CXXConstructorDecl& decl) {
   assert_non_template(decl);
   wrap_non_template(ctx, decl);
}

fn wrap_non_template(Context& ctx, const clang::CXXMethodDecl& decl) {
   if (ctx.access_guard(decl)) {
      ctx.add(node<RoutineDecl>(MethodDecl(
          decl.getNameAsString(), decl.getQualifiedNameAsString(),
          expect(ctx.header(decl), "failed to canonicalize source location"),
          map(ctx, decl.getParent()), wrap_formals(ctx, decl), wrap_return_type(ctx, decl))));
   }
}

fn wrap(Context& ctx, const clang::CXXMethodDecl& decl) {
   assert_non_template(decl);
   wrap_non_template(ctx, decl);
}

fn wrap(Context& ctx, const clang::EnumDecl& decl) {
   if (ctx.access_guard(decl)) {
      ctx.add(node<TypeDecl>(
          EnumTypeDecl(qualified_nim_name(ctx, decl), decl.getQualifiedNameAsString(),
                       expect(ctx.header(decl), "failed to canonicalize source location"),
                       transfer(ctx, decl.enumerators()))));
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

/// The main executor of ensnare.
///
/// Responsible for building the translation unit, invoking the visitor, and writing the output.
class Main : public clang::RecursiveASTVisitor<Main> {
   priv const Config& cfg;
   priv const std::unique_ptr<clang::ASTUnit> tu;
   priv Context ctx;

   fn prefixed_search_paths() -> Vec<Str> {
      Vec<Str> result;
      for (const auto& search_path : Header::search_paths()) {
         result.push_back("-isystem=" + search_path);
      }
      return result;
   }

   // Load a translation unit from user provided arguments with additional include path arguments.
   priv fn parse_tu() -> std::unique_ptr<clang::ASTUnit> {
      Vec<Str> args = {"-xc++"};
      auto search_paths = prefixed_search_paths();
      args.insert(args.end(), search_paths.begin(), search_paths.end());
      // The users args get placed after for higher priority.
      args.insert(args.end(), cfg.user_clang_args().begin(), cfg.user_clang_args().end());
      return clang::tooling::buildASTFromCodeWithArgs(cfg.header_file(), args, "ensnare_headers.h",
                                                      "ensnare");
   }

   /// Render and write out our decls.
   priv fn finalize() {
      Str output = "import ensnare/runtime\n";
      output += render(ctx.type_decls());
      output += render(ctx.routine_decls());
      output += render(ctx.variable_decls());
      os::write_file(os::set_file_ext(ctx.cfg.output(), ".nim"), output);
   }

   pub Main(const Config& cfg) : cfg(cfg), tu(parse_tu()), ctx(cfg, tu->getASTContext()) {
      // Collect all our declarations.
      auto result = TraverseAST(tu->getASTContext());
      if (!result) { // Idk wtf this thing means? It is true even with errors...
         fatal("Main failed");
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

/// Entrypoint to the c++ part of ensnare.
void run(int argc, const char* argv[]) { Main _(Config(argc, argv)); }
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

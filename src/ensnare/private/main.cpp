#include "ensnare/private/main.hpp"

#include "ensnare/private/bit_utils.hpp"
#include "ensnare/private/builtins.hpp"
#include "ensnare/private/clang_utils.hpp"
#include "ensnare/private/config.hpp"
#include "ensnare/private/decl.hpp"
#include "ensnare/private/header_canonicalizer.hpp"
#include "ensnare/private/headers.hpp"
#include "ensnare/private/render.hpp"
#include "ensnare/private/runtime.hpp"
#include "ensnare/private/str_utils.hpp"
#include "ensnare/private/sym_generator.hpp"
#include "ensnare/private/utils.hpp"

/* FIXME: c++ template methods with explicit arguments
template <std::size_t size, typename T> class Vec {
   template <typename U> int some_meth(T val);
}

proc some_meth*[size: static[CppSize]; T; U](�: Vec[size, T], val: T): CppInt

Consider the template method where the `U` template parameter cannot be inferred.
In nim we would like to call it like some_vec.some_meth[:int](2.0) where the first two are inferred.
Nim generic instantiations seem to be all or nothing so we can't do that. Instead we should be
determine if a template parameter can be inferred. If it can't we insert typedesc arguments.

proc some_meth*[size: static[CppSize]; T; U](�: Vec[size, T], �1: type[U], val: T): CppInt

Calling it like: some_vec.some_meth(int, 2.0)
*/

/* FIXME: SFINAE
Much of <type_trait> can be translated to nim's generic constaints.
How to determine user defined type traits?
Recurse each template parameter / expression and check if it is a type trait.
Similar treatment for return type SFINAE. User input for handling any misclassifications
*/

/* FIXME: Impliment a target system for cross compilation and arch specific features/builtins.
Cross compilation is already exposed with `--disable-include-search-paths` and clang's `--target`.
*/

/* FIXME: preprocessor imrofation
clang offers some PP information that could be exposed as constants and templates.
*/

// FIXME: Introspection is key to producing nice wrappers. For that we need a more procedural api.

// FIXME: map documentation comments

// FIXME: template variables

// FIXME: namespace handling

/* FIXME: rvalue reference handling is broken.
consider:
void func(Foo&& blah) {}
void func(Foo& blah) {}
void func(Foo blah) {}

These will produce a nim redefinition error because rvalue refernences are transparent.
Simply ignoring functions with rvalue ref loses information if the function is not overloaded.
We may have to simulate overloads ourselves.
Perhaps using the nim compiler's machinery.
*/

/* FIXME: Avoiding rebinding the world.
We want to be able to load already bound libraries and reference those.
Using the compiler again?
*/

namespace ensnare {
/// Holds ensnare's state.
///
/// A `Context` must not outlive a `Config`
class Context {
   public:
   const clang::ASTContext& ast_ctx; ///< For accessing source location information.

   private:
   Map<const clang::Decl*, Type> type_lookup; ///< Maps clang declarations to already bound types.
   public:
   Map<const clang::Decl*, Node<TemplateParam>> templ_params;

   private:
   Vec<TypeDecl> _type_decls;
   Vec<RoutineDecl> _routine_decls;
   Vec<VariableDecl> _variable_decls;

   HeaderCanonicalizer header_canonicalizer; ///< Each declaration we bind must have a header to
                                             ///< otherwise we would get nim backend errors.
                                             ///< The header must be relative to an include path.
                                             ///< This handles that logic.

   public:
   /// The types we have bound.
   const Vec<TypeDecl>& type_decls() const { return _type_decls; }
   /// The routines we have bound.
   const Vec<RoutineDecl>& routine_decls() const { return _routine_decls; }
   /// The variables we have bound.
   const Vec<VariableDecl>& variable_decls() const { return _variable_decls; }

   private:
   Vec<const clang::NamedDecl*> decl_stack; ///< To give anonymous tags useful names, we track
                                            ///< the declaration they were referenced from.
   public:
   const Config& cfg;

   Context(const Config& cfg, const clang::ASTContext& ast_ctx)
      : cfg(cfg), ast_ctx(ast_ctx), header_canonicalizer(cfg, ast_ctx.getSourceManager()) {}

   /// See Context::decl_stack.
   const clang::NamedDecl& decl(int i) const {
      if (i < decl_stack.size()) {
         return *decl_stack[decl_stack.size() - i - 1];
      } else {
         for (auto decl : decl_stack) {
            write(render(decl));
         }
         fatal("unreachable: failed to get decl context");
      }
   }

   /// See Context::decl_stack.
   void push(const clang::NamedDecl& decl) { decl_stack.push_back(&decl); }
   /// See Context::decl_stack.
   void pop_decl() { decl_stack.pop_back(); }

   private:
   /// We want the forward declarations and defintions to resolve to the same entity, otherwise
   /// weird shit happend. Anything that we use for lookups must go through here.
   const clang::Decl& canon_lookup_decl(const clang::Decl& decl) const {
      if (auto tag_decl = llvm::dyn_cast<clang::TagDecl>(&decl)) {
         return get_definition(*tag_decl);
      } else {
         return decl;
      }
   }

   public:
   /// Lookup the Type of a declaration if any exists.
   Opt<Type> lookup(const clang::Decl& decl) const {
      auto decl_type = type_lookup.find(&canon_lookup_decl(decl));
      if (decl_type == type_lookup.end()) {
         return {};
      } else {
         return decl_type->second;
      }
   }

   /// Record the Type of a declaration for future mapping.
   void associate(const clang::Decl& decl, Type type) {
      auto& key = canon_lookup_decl(decl);
      auto maybe_type = lookup(key);
      if (maybe_type) {
         write(render(decl));
         fatal("unreachable: duplicate type in lookup");
      } else {
         type_lookup[&key] = type;
      }
   }

   /// Add a type declaration to be rendered.
   void add(const TypeDecl decl) { _type_decls.push_back(decl); }

   /// Add a routine declaration to be rendered.
   void add(const RoutineDecl decl) { _routine_decls.push_back(decl); }

   /// Add a variable declaration to be rendered.
   void add(const VariableDecl decl) { _variable_decls.push_back(decl); }

   /// Filters access to protected and private members.
   bool access_guard(const clang::Decl& decl) const {
      return decl.getAccess() == clang::AS_public || decl.getAccess() == clang::AS_none;
   }

   /// See Context::header_canonicalizer
   Opt<Str> maybe_header(const clang::NamedDecl& decl) {
      return header_canonicalizer[decl.getLocation()];
   }

   /// See Context::header_canonicalizer
   Str header(const clang::NamedDecl& decl) {
      auto h = maybe_header(decl);
      if (h) {
         return *h;
      } else {
         write(render(decl));
         fatal("failed to canonicalize source location");
      }
   }
};

void force_wrap(Context& ctx, const clang::NamedDecl& decl);

Type map(Context& ctx, const clang::Type& decl);

/// Generic map for pointers to error on null.
template <typename T> Type map(Context& ctx, const T* entity) {
   if (entity) {
      return map(ctx, *entity);
   } else {
      fatal("tried to map a null entity");
   }
}

/// Maps an atomic type of some kind. Many of these are obscure and unsupported.
Type map(Context& ctx, const clang::BuiltinType& entity) {
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
      case clang::BuiltinType::Kind::SatULongFract: fatal("c11 builtins unsupported");
      case clang::BuiltinType::Kind::ObjCId:
      case clang::BuiltinType::Kind::ObjCClass:
      case clang::BuiltinType::Kind::ObjCSel: fatal("obj-c builtins unsupported");
      case clang::BuiltinType::Kind::Char_S: fatal("unreachable: implicitly signed char");
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
      case clang::BuiltinType::Kind::UChar: return builtins::_uchar;
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
Type map(Context& ctx, const clang::QualType& entity) {
   // Too niche to care about for now.
   //  if (entity.isRestrictQualified() || entity.isVolatileQualified()) {
   //    fatal("volatile and restrict qualifiers unsupported");
   // }
   auto type = entity.getTypePtr();
   if (type) {
      auto result = map(ctx, *type);
      // FIXME: `isLocalConstQualified`: do we care about local vs non-local?
      if (entity.isConstQualified() && !ctx.cfg.ignore_const()) {
         return new_Type(ConstType(result));
      } else {
         return result;
      }
   } else {
      fatal("QualType inner type was null");
   }
}

/// This maps certain kinds of declarations by first trying to look them up in the context.
/// If it is not in the context, we enforce wrapping it (or fail binding) and return the mapped
/// result.
Type map(Context& ctx, const clang::NamedDecl& decl) {
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

void update_templ_param(Context& ctx, const clang::NamedDecl& named_decl) {
   // These do not need to go through canon_lookup since they cannot be forward declared.
   if (ctx.templ_params.count(&named_decl) == 0) {
      auto sym = new_Sym(named_decl.getNameAsString());
      switch (named_decl.getKind()) {
         case clang::Decl::Kind::TemplateTypeParm: {
            auto& decl = llvm::cast<clang::TemplateTypeParmDecl>(named_decl);
            ctx.associate(decl, new_Type(sym));
            ctx.templ_params[&decl] = new_TemplateParam(sym);
            break;
         }
         case clang::Decl::Kind::NonTypeTemplateParm: {
            auto& decl = llvm::cast<clang::NonTypeTemplateParmDecl>(named_decl);
            ctx.associate(decl, new_Type(sym));
            ctx.templ_params[&decl] = node<TemplateParam>(
                sym,
                new_Type(InstType(new_Type(new_Sym("static", true)), {map(ctx, decl.getType())})));
            break;
         }
         default: write(render(named_decl)); fatal("unhandled template argument");
      }
   }
}

Node<TemplateParam> wrap_templ_param(Context& ctx, const clang::NamedDecl& decl) {
   update_templ_param(ctx, decl);
   return ctx.templ_params[&decl];
}

Type map_templ_param(Context& ctx, const clang::NamedDecl& decl) {
   update_templ_param(ctx, decl);
   return map(ctx, decl);
}

Expr map_expr(Context& ctx, const clang::Expr& expr) {
   switch (expr.getStmtClass()) {
      // This should be a non type template parameter. Probably innacurate now.
      case clang::Stmt::DeclRefExprClass: {
         auto templ_param = wrap_templ_param(ctx, *llvm::cast<clang::DeclRefExpr>(expr).getDecl());
         return new_Expr(ConstParamExpr(templ_param->name));
      }
      case clang::Stmt::IntegerLiteralClass: {
         auto& int_expr = llvm::cast<clang::IntegerLiteral>(expr);
         // FIXME: this should handle U64 too by checking the type.
         return new_Expr(LitExpr<I64>(int_expr.getValue().getSExtValue()));
      }
      default: write(render(expr)); fatal("unhandled map_expr");
   }
}

Type inst_name_type(Context& ctx, const clang::TemplateName& name) {
   switch (name.getKind()) {
      case clang::TemplateName::Template: return map(ctx, name.getAsTemplateDecl());
      case clang::TemplateName::OverloadedTemplate:
      case clang::TemplateName::AssumedTemplate:
      case clang::TemplateName::QualifiedTemplate:
      case clang::TemplateName::DependentTemplate:
      case clang::TemplateName::SubstTemplateTemplateParm:
      case clang::TemplateName::SubstTemplateTemplateParmPack:
      default: print(render(name)); fatal("unhandled template name");
   }
}

template <typename Y, typename X> Type fold_func(Context& ctx, const X& type) {
   // if (ty.isFunctionPointerType()) { // the ptr part is implicit for nim.
   auto pointee = type.getPointeeType();
   auto pointee_result = map(ctx, pointee);
   if (pointee->isFunctionType()) {
      return pointee_result;
   } else {
      return new_Type(Y(pointee_result));
   }
}

template <typename T> Opt<Type> map_return_type(Context& ctx, const T& entity) {
   auto result = entity.getReturnType();
   if (result->isVoidType()) {
      return {};
   } else {
      return map(ctx, result);
   }
}

Type map(Context& ctx, const clang::Type& type) {
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
      case clang::Type::TemplateTypeParm:
         return map_templ_param(ctx, *llvm::cast<clang::TemplateTypeParmType>(type).getDecl());
      case clang::Type::TypeClass::Pointer: {
         auto& ty = llvm::cast<clang::PointerType>(type);
         if (ty.isVoidPointerType()) { // `void*` is `pointer`
            return builtins::_void_ptr;
         } else {
            return fold_func<PtrType>(ctx, ty);
         }
      }
      case clang::Type::TypeClass::RValueReference:
         return map(ctx, llvm::cast<clang::ReferenceType>(type).getPointeeType());
      case clang::Type::TypeClass::LValueReference:
         return fold_func<RefType>(ctx, llvm::cast<clang::ReferenceType>(type));
      case clang::Type::TypeClass::Vector:
         // The hope is that this is an aliased type as part of `typedef` / `using`
         // and the internals don't matter.
         return new_Type(OpaqueType());
      case clang::Type::TypeClass::ConstantArray: {
         auto& ty = llvm::cast<clang::ConstantArrayType>(type);
         return new_Type(ArrayType(new_Expr(LitExpr(ty.getSize().getLimitedValue())),
                                   map(ctx, ty.getElementType())));
      }
      case clang::Type::TypeClass::DependentSizedArray: {
         auto& ty = llvm::cast<clang::DependentSizedArrayType>(type);
         return new_Type(
             ArrayType(map_expr(ctx, *ty.getSizeExpr()), map(ctx, ty.getElementType())));
      }
      case clang::Type::TypeClass::IncompleteArray:
         return new_Type(UnsizedArrayType(
             map(ctx, llvm::cast<clang::IncompleteArrayType>(type).getElementType())));
      case clang::Type::TypeClass::FunctionProto: {
         const auto& ty = llvm::cast<clang::FunctionProtoType>(type);
         Vec<Type> types;
         for (auto param_type : ty.param_types()) {
            types.push_back(map(ctx, param_type));
         }
         return new_Type(FuncType(types, map_return_type(ctx, ty)));
      }
      case clang::Type::TypeClass::Decltype:
         return map(ctx, llvm::cast<clang::DecltypeType>(type).getUnderlyingType());
      case clang::Type::TypeClass::Paren:
         return map(ctx, llvm::cast<clang::ParenType>(type).getInnerType());
      case clang::Type::TypeClass::Builtin: return map(ctx, llvm::cast<clang::BuiltinType>(type));
      case clang::Type::TemplateSpecialization: {
         const auto& ty = llvm::cast<clang::TemplateSpecializationType>(type);
         auto name = ty.getTemplateName();
         Vec<InstType::Arg> args;
         for (auto arg : ty.template_arguments()) {
            switch (arg.getKind()) {
               case clang::TemplateArgument::Type:
                  args.emplace_back(map(ctx, arg.getAsType()));
                  break;
               case clang::TemplateArgument::Expression:
                  args.emplace_back(map_expr(ctx, *arg.getAsExpr()));
                  break;
               case clang::TemplateArgument::Null:
               case clang::TemplateArgument::Declaration:
               case clang::TemplateArgument::NullPtr:
               case clang::TemplateArgument::Integral:
               case clang::TemplateArgument::Template:
               case clang::TemplateArgument::TemplateExpansion:
               case clang::TemplateArgument::Pack:
               default:
                  print(render(arg));
                  fatal("unhandled inst template argument: ", render(arg.getKind()));
            }
         }
         return new_Type(InstType(inst_name_type(ctx, ty.getTemplateName()), args));
      }
      default: print(render(type)); fatal("unhandled mapping: ", type.getTypeClassName());
   }
} // namespace ensnare

Param make_param(Context& ctx, const clang::ParmVarDecl& param) {
   auto name = new_Sym(param.getNameAsString());
   auto type = map(ctx, param.getType());
   if (param.hasDefaultArg()) {
      return Param(name, type, map_expr(ctx, *param.getDefaultArg()));
   } else {
      return Param(name, type);
   }
}

Params params(Context& ctx, const clang::FunctionDecl& decl) {
   Params result;
   for (auto param : decl.parameters()) {
      result.push_back(make_param(ctx, *param));
   }
   return result;
}

void wrap_function(Context& ctx, const clang::FunctionDecl& decl) {
   ctx.add(new_RoutineDecl(FunctionDecl(decl.getNameAsString(), qual_name(decl), ctx.header(decl),
                                        params(ctx, decl), map_return_type(ctx, decl))));
}

Sym type_sym(Type type) {
   require(is<Sym>(type), "Type is not a Sym");
   return as<Sym>(type);
}

TemplateParams template_params(Context& ctx, const clang::TemplateDecl& decl) {
   TemplateParams result;
   for (auto argument : *decl.getTemplateParameters()) {
      result.push_back(wrap_templ_param(ctx, *argument));
   }
   return result;
}

void wrap_template_function(Context& ctx, const clang::FunctionDecl& decl) {
   ctx.add(new_RoutineDecl(FunctionDecl(decl.getNameAsString(), qual_name(decl), ctx.header(decl),
                                        template_params(ctx, *decl.getDescribedFunctionTemplate()),
                                        params(ctx, decl), map_return_type(ctx, decl))));
}

void wrap_function_base(Context& ctx, const clang::FunctionDecl& decl) {
   switch (decl.getTemplatedKind()) {
      case clang::FunctionDecl::TK_NonTemplate: wrap_function(ctx, decl); break;
      case clang::FunctionDecl::TK_FunctionTemplate: wrap_template_function(ctx, decl); break;
      case clang::FunctionDecl::TK_MemberSpecialization:
      case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization:
         print(render(decl));
         fatal("only simple function signatures are supported.");
   }
}

// For methods there are possibly template parameters coming from the class and the method
// itself. The same applies for other class local decls like ctor and dtor.

Opt<TemplateParams> self_template_params(Context& ctx, const clang::CXXRecordDecl& decl) {
   if (auto templ_decl = decl.getDescribedClassTemplate()) {
      return template_params(ctx, *templ_decl);
   } else {
      return {};
   }
}

Str ctor_import_name(Context& ctx, const clang::CXXConstructorDecl& decl) { return "'0"; }

Type self_type(Context& ctx, const clang::CXXMethodDecl& decl) {
   auto& parent = *decl.getParent();
   auto type = map(ctx, parent);
   if (auto templ = parent.getDescribedClassTemplate()) {
      Vec<InstType::Arg> params;
      for (auto argument : *templ->getTemplateParameters()) {
         params.push_back(InstType::Arg(map(ctx, *argument)));
      }
      return new_Type(InstType(type, params));
   } else {
      return type;
   }
}

void wrap_ctor(Context& ctx, const clang::CXXConstructorDecl& decl) {
   // We don't wrap anonymous constructors since they take a supposedly anonymous type as a
   // parameter.
   if (ctx.access_guard(decl) && has_name(*decl.getParent())) {
      ctx.add(new_RoutineDecl(ConstructorDecl(ctor_import_name(ctx, decl), ctx.header(decl),
                                              self_template_params(ctx, *decl.getParent()),
                                              self_type(ctx, decl), params(ctx, decl))));
   }
}

void wrap_template_ctor(Context& ctx, const clang::CXXConstructorDecl& decl) {
   ctx.add(new_RoutineDecl(ConstructorDecl(
       ctor_import_name(ctx, decl), ctx.header(decl), self_template_params(ctx, *decl.getParent()),
       self_type(ctx, decl), template_params(ctx, *decl.getDescribedFunctionTemplate()),
       params(ctx, decl))));
}

void wrap_ctor_base(Context& ctx, const clang::CXXConstructorDecl& decl) {
   switch (decl.getTemplatedKind()) {
      case clang::FunctionDecl::TK_NonTemplate: wrap_ctor(ctx, decl); break;
      case clang::FunctionDecl::TK_FunctionTemplate: wrap_template_ctor(ctx, decl); break;
      case clang::FunctionDecl::TK_MemberSpecialization:
      case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization:
         print(render(decl));
         fatal("only simple function signatures are supported.");
   }
}

Str method_import_name(Context& ctx, const clang::CXXMethodDecl& decl) {
   return (decl.isStatic() ? "'0::" : "#.") + decl.getNameAsString();
}

Opt<Str> operator_name(const clang::DeclarationName& name) {
   switch (name.getCXXOverloadedOperator()) {
      case clang::OO_Subscript: return "[]";
      case clang::OO_Plus: return "+";
      case clang::OO_PlusEqual: return "+=";
      case clang::OO_Minus: return "-";
      case clang::OO_MinusEqual: return "-=";
      case clang::OO_Star: return "*";
      case clang::OO_StarEqual: return "*=";
      case clang::OO_Slash: return "/";
      case clang::OO_SlashEqual: return "/=";
      case clang::OO_Percent: return "%";
      case clang::OO_PercentEqual: return "%=";
      case clang::OO_Caret: return "^";
      case clang::OO_CaretEqual: return "^=";
      case clang::OO_GreaterGreater: return ">>";
      case clang::OO_GreaterGreaterEqual: return ">>=";
      case clang::OO_LessLess: return "<<";
      case clang::OO_LessLessEqual: return "<<=";
      case clang::OO_Pipe: return "|";
      case clang::OO_PipeEqual: return "|=";
      case clang::OO_Amp: return "&";
      case clang::OO_AmpEqual: return "&=";
      case clang::OO_AmpAmp: return "&&";
      case clang::OO_PipePipe: return "||";
      case clang::OO_Exclaim: return "!";
      case clang::OO_Less: return "<";
      case clang::OO_Greater: return ">";
      case clang::OO_EqualEqual: return "==";
      case clang::OO_ExclaimEqual: return "!=";
      case clang::OO_LessEqual: return "<=";
      case clang::OO_GreaterEqual: return ">=";
      case clang::OO_Spaceship: return "<=>";
      case clang::OO_Tilde: return "~";
      case clang::OO_New: return "cpp_new";
      case clang::OO_Delete: return "cpp_delete";
      case clang::OO_Array_New: return "cpp_array_new";
      case clang::OO_Array_Delete: return "cpp_array_delete";
      case clang::OO_None:
      case clang::OO_PlusPlus:
      case clang::OO_MinusMinus:
      case clang::OO_ArrowStar:
      case clang::OO_Arrow:
      case clang::OO_Call:
      case clang::OO_Conditional:
      case clang::OO_Coawait:
      case clang::OO_Equal:
      case clang::OO_Comma:
      default: return {};
   }
}

Str method_name(Context& ctx, const clang::CXXMethodDecl& decl) {
   auto name = decl.getDeclName();
   switch (name.getNameKind()) {
      case clang::DeclarationName::Identifier: return name.getAsIdentifierInfo()->getName();
      case clang::DeclarationName::CXXOperatorName:
         if (auto op_name = operator_name(name)) {
            return *op_name;
         } else {
            return decl.getNameAsString();
         }
      case clang::DeclarationName::CXXConstructorName:
      case clang::DeclarationName::CXXDestructorName:
      case clang::DeclarationName::CXXConversionFunctionName:
      case clang::DeclarationName::CXXDeductionGuideName:
      case clang::DeclarationName::CXXLiteralOperatorName:
      case clang::DeclarationName::CXXUsingDirective:
         name.dump();
         fatal("unhandled declaration name");
      case clang::DeclarationName::ObjCZeroArgSelector:
      case clang::DeclarationName::ObjCOneArgSelector:
      case clang::DeclarationName::ObjCMultiArgSelector: fatal("obj-c unsupported");
   }
}

void wrap_method(Context& ctx, const clang::CXXMethodDecl& decl) {
   if (ctx.access_guard(decl)) {
      ctx.add(new_RoutineDecl(
          MethodDecl(method_name(ctx, decl), method_import_name(ctx, decl), ctx.header(decl),
                     self_template_params(ctx, *decl.getParent()), self_type(ctx, decl),
                     params(ctx, decl), map_return_type(ctx, decl), decl.isStatic())));
   }
}

void wrap_template_method(Context& ctx, const clang::CXXMethodDecl& decl) {
   if (ctx.access_guard(decl)) {
      ctx.add(new_RoutineDecl(
          MethodDecl(method_name(ctx, decl), method_import_name(ctx, decl), ctx.header(decl),
                     self_template_params(ctx, *decl.getParent()), self_type(ctx, decl),
                     template_params(ctx, *decl.getDescribedFunctionTemplate()), params(ctx, decl),
                     map_return_type(ctx, decl), decl.isStatic())));
   }
}

void wrap_method_base(Context& ctx, const clang::CXXMethodDecl& decl) {
   // FIXME: const self parameters
   switch (decl.getTemplatedKind()) {
      case clang::FunctionDecl::TK_NonTemplate: wrap_method(ctx, decl); break;
      case clang::FunctionDecl::TK_FunctionTemplate: wrap_template_method(ctx, decl); break;
      case clang::FunctionDecl::TK_MemberSpecialization:
      case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization:
         print(render(decl));
         fatal("only simple function signatures are supported.");
   }
}

void wrap_conv_base(Context& ctx, const clang::CXXConversionDecl& decl) {
   print(render(decl));
   fatal("FIXME: wrap_conv_base");
}

void maybe_wrap_method(Context& ctx, const clang::CXXMethodDecl& decl) {
   if (decl.getKind() == clang::Decl::Kind::CXXMethod) {
      wrap_method_base(ctx, decl);
   } else if (auto constructor = llvm::dyn_cast<clang::CXXConstructorDecl>(&decl)) {
      wrap_ctor_base(ctx, *constructor);
   } else if (auto conversion = llvm::dyn_cast<clang::CXXConversionDecl>(&decl)) {
      wrap_conv_base(ctx, *conversion);
   } else if (llvm::isa<clang::CXXDestructorDecl>(decl)) {
      // We don't wrap destructors to make it harder for fools to call them manually.
   } else {
      print(render(decl));
      fatal("unreachable: wrap(CXXRecordDecl::method_range)");
   }
}

void wrap_methods(Context& ctx, const clang::CXXRecordDecl& decl) {
   for (auto child_decl : decl.decls()) {
      // manual check because isa considers descendents too.
      if (auto method = llvm::dyn_cast<clang::CXXMethodDecl>(child_decl)) {
         maybe_wrap_method(ctx, *method);
      } else if (auto templ = llvm::dyn_cast<clang::FunctionTemplateDecl>(child_decl)) {
         maybe_wrap_method(ctx, llvm::cast<clang::CXXMethodDecl>(*templ->getTemplatedDecl()));
      } else {
         // do nothing
      }
   }
}

/// A qualified name contains all opaque namespace and type contexts.
/// Instead of the c++ double colon we use a minus and stropping.
Str qual_nim_name(Context& ctx, const clang::NamedDecl& decl) {
   return replace(qual_name(decl), "::", "-");
}

Vec<RecordFieldDecl> transfer(Context& ctx, const clang::CXXRecordDecl::field_range fields) {
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

Sym tag_name(Context& ctx, const clang::NamedDecl& decl) {
   return has_name(decl) ? new_Sym(qual_nim_name(ctx, decl))
                         : new_Sym("type_of(" + qual_nim_name(ctx, ctx.decl(1)) + ")");
}

Sym register_tag_name(Context& ctx, const clang::NamedDecl& name_decl,
                      const clang::TagDecl& def_decl) {
   auto result = tag_name(ctx, name_decl);
   auto type = new_Type(result);
   ctx.associate(name_decl, type);
   if (!pointer_equality(name_decl, llvm::cast<clang::NamedDecl>(def_decl))) {
      ctx.associate(def_decl, type);
   }
   return result;
}

// FIXME: clean this up, also the tag is probably not needed for templates.
Sym register_templ_record_name(Context& ctx, const clang::NamedDecl& name_decl,
                               const clang::TagDecl& def_decl,
                               const clang::ClassTemplateDecl& templ_def_decl) {
   auto result = tag_name(ctx, name_decl);
   auto type = new_Type(result);
   ctx.associate(name_decl, type);
   if (!pointer_equality(name_decl, llvm::cast<clang::NamedDecl>(def_decl))) {
      ctx.associate(def_decl, type);
   }
   ctx.associate(templ_def_decl, type);
   return result;
}

Str tag_import_name(Context& ctx, const clang::NamedDecl& decl) {
   return has_name(decl) ? decl.getQualifiedNameAsString()
                         : "decltype(" + ctx.decl(1).getQualifiedNameAsString() + ")";
}

void wrap_record_non_template(Context& ctx, const clang::NamedDecl& name_decl,
                              const clang::CXXRecordDecl& def_decl, bool force) {
   ctx.add(new_TypeDecl(RecordTypeDecl(register_tag_name(ctx, name_decl, def_decl),
                                       tag_import_name(ctx, name_decl), ctx.header(name_decl),
                                       transfer(ctx, def_decl.fields()))));
   if (!force) {
      wrap_methods(ctx, def_decl);
   }
}

Str template_record_import_name(Context& ctx, const clang::ClassTemplateDecl& templ_def_decl) {
   Str result = templ_def_decl.getQualifiedNameAsString() + "<";
   for (auto i = 0; i < template_params(ctx, templ_def_decl).size(); i += 1) {
      if (i != 0) {
         result += ", ";
      }
      result += "'" + std::to_string(i);
   }
   result += ">";
   return result;
}

void wrap_record_template(Context& ctx, const clang::NamedDecl& name_decl,
                          const clang::CXXRecordDecl& def_decl,
                          const clang::ClassTemplateDecl& templ_def_decl, bool force) {
   ctx.add(new_TypeDecl(
       RecordTypeDecl(register_templ_record_name(ctx, name_decl, def_decl, templ_def_decl),
                      template_record_import_name(ctx, templ_def_decl), ctx.header(name_decl),
                      template_params(ctx, templ_def_decl), transfer(ctx, def_decl.fields()))));
   if (!force) {
      wrap_methods(ctx, def_decl);
   }
}

void wrap_record(Context& ctx, const clang::NamedDecl& name_decl,
                 const clang::CXXRecordDecl& def_decl, bool force) {
   if (has_name(name_decl) || force) {
      if (auto templ_def_decl = def_decl.getDescribedClassTemplate()) {
         wrap_record_template(ctx, name_decl, def_decl, *templ_def_decl, force);
      } else {
         wrap_record_non_template(ctx, name_decl, def_decl, force);
      }
   }
}

void wrap_enum(Context& ctx, const clang::NamedDecl& name_decl, const clang::EnumDecl& def_decl,
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
             EnumFieldDecl(new_Sym(field->getNameAsString()), field->getInitVal().getExtValue()));
      }
      ctx.add(new_TypeDecl(EnumTypeDecl(register_tag_name(ctx, name_decl, def_decl),
                                        tag_import_name(ctx, name_decl), ctx.header(name_decl),
                                        fields)));
   }
}

void wrap_tag(Context& ctx, const clang::NamedDecl& name_decl, const clang::TagDecl& def_decl,
              bool force) {
   // clang::RecordDecl is omitted because it does not exist in c++.
   auto kind = def_decl.getKind();
   if (kind == clang::Decl::Kind::CXXRecord) {
      wrap_record(ctx, name_decl, llvm::cast<clang::CXXRecordDecl>(def_decl), force);
   } else if (kind == clang::Decl::Kind::Enum) {
      wrap_enum(ctx, name_decl, llvm::cast<clang::EnumDecl>(def_decl), force);
   } else {
      write(render(def_decl));
      fatal("unhandled: anon tag typedef");
   }
}

void wrap_tag(Context& ctx, const clang::TagDecl& def_decl, bool force) {
   wrap_tag(ctx, def_decl, def_decl, force);
}

/// Check what declaration this is from <cstddef> if any.
/// FIXME: Special casing these is useful since they are so common.
///        The same treatment should be given to <cstdint> and maybe others.
///        This should be replaced with a more general purpose mechanism.
Opt<Type> get_cstddef_item(Context& ctx, const clang::NamedDecl& decl) {
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

void wrap_simple_alias(Context& ctx, const clang::TypedefNameDecl& decl) {
   auto name = new_Sym(qual_nim_name(ctx, decl));
   ctx.associate(decl, new_Type(name));
   auto type_decl = new_TypeDecl(AliasTypeDecl(name, map(ctx, decl.getUnderlyingType())));
   ctx.add(type_decl);
}

bool t_suffix_eq(Context& ctx, const clang::NamedDecl& type_t, const Str& type) {
   return ctx.cfg.fold_type_suffix() && qual_name(type_t) == type + "_t";
}

void add_t_suffix(Context& ctx, const clang::TypedefDecl& decl) {
   auto name = type_sym(map(ctx, decl));
   if (!ends_with(name->latest(), "_t")) {
      name->update(name->latest() + "_t");
   }
}

void wrap_tag_typedef(Context& ctx, const clang::TypedefDecl& name_decl,
                      const clang::TagDecl& def_decl, bool owns_tag) {
   // FIXME: The force parameters here are wrong. We want to force the typedef bindings but
   //        not the associated mathods and types. Perhaps we always want to force typedef rhs?
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

void wrap_typedef(Context& ctx, const clang::TypedefDecl& decl) {
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

/// For every type with a name we must produce an accessible type declartion for nim.
/// This forces binding declarations of types we did not previously bind.
/// It is called during mapping.
void force_wrap(Context& ctx, const clang::NamedDecl& named_decl) {
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
         case clang::Decl::Kind::ClassTemplate:
            wrap_tag(
                ctx,
                get_definition(llvm::cast<clang::ClassTemplateDecl>(named_decl).getTemplatedDecl()),
                true);
         default:
            write(render(named_decl));
            fatal("unhandled force_wrap: ", named_decl.getDeclKindName(), "Decl");
      }
      ctx.pop_decl();
   }
}

void wrap(Context& ctx, const clang::NamedDecl& named_decl) {
   /// Don't double wrap and only wrap things we can actually give a source location too.
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
            wrap_function_base(ctx, llvm::cast<clang::FunctionDecl>(named_decl));
            break;
         case clang::Decl::Kind::Var: {
            auto& decl = llvm::cast<clang::VarDecl>(named_decl);
            if (ctx.access_guard(decl) && decl.hasGlobalStorage() && !decl.isStaticLocal()) {
               ctx.add(new_VariableDecl(qual_nim_name(ctx, decl), qual_name(decl), ctx.header(decl),
                                        map(ctx, decl.getType())));
            }
            break;
         }
         case clang::Decl::Kind::ObjCIvar:
         case clang::Decl::Kind::ObjCAtDefsField: fatal("obj-c not supported");
         case clang::Decl::Kind::Namespace: // FIXME: doing something with this would be good.
         case clang::Decl::Kind::Using:
         case clang::Decl::Kind::UsingDirective:
         case clang::Decl::Kind::NamespaceAlias:
         case clang::Decl::Kind::Field:
         case clang::Decl::Kind::ParmVar:
         case clang::Decl::Kind::TemplateTypeParm:
         case clang::Decl::Kind::NonTypeTemplateParm:
         case clang::Decl::Kind::EnumConstant:
         case clang::Decl::Kind::CXXMethod:
         case clang::Decl::Kind::CXXConstructor:
         case clang::Decl::Kind::CXXDestructor:
         case clang::Decl::Kind::ClassTemplate:
         case clang::Decl::Kind::FunctionTemplate: break; // discarded
         default:
            write(render(named_decl));
            fatal("unhandled wrapping: ", named_decl.getDeclKindName(), "Decl");
      }
      ctx.pop_decl();
   }
}

/// The base wrapping routine. Only filters out unnamed decls.
void base_wrap(Context& ctx, const clang::Decl& decl) {
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
      case clang::Decl::Kind::TranslationUnit: break; // discarded
      case clang::Decl::Kind::firstNamed... clang::Decl::Kind::lastNamed: {
         wrap(ctx, llvm::cast<clang::NamedDecl>(decl));
         break;
      }
      default: write(render(decl)); fatal("unhandled wrapping: ", decl.getDeclKindName(), "Decl");
   }
}

/// Procduce system search path arguments
Vec<Str> prefixed_search_paths() {
   Vec<Str> result;
   for (const auto& search_path : Header::search_paths()) {
      result.push_back("-isystem" + search_path);
   }
   return result;
}

/// Load a translation unit from user provided arguments with additional include path
/// arguments.
std::unique_ptr<clang::ASTUnit> parse_translation_unit(const Config& cfg) {
   // We target c++ and we mimic nim's semantics of default unsigned chars.
   Vec<Str> args = {"-xc++", "-funsigned-char"};
   auto search_paths = prefixed_search_paths();
   args.insert(args.end(), search_paths.begin(), search_paths.end());
   for (auto include_dir : cfg.include_dirs()) {
      args.push_back("-I" + include_dir);
   }
   // The users args get placed after for higher priority.
   args.insert(args.end(), cfg.user_clang_args().begin(), cfg.user_clang_args().end());
   return clang::tooling::buildASTFromCodeWithArgs(cfg.header_file(), args, "ensnare_headers.hpp",
                                                   "ensnare");
}

/// Performs:
///    gensyming of types to deal with the struct namespace.
void post_process(Context& ctx) {
   SymGenerator gensym;
   for (auto& type_decl : ctx.type_decls()) {
      for (auto& gensym_type : ctx.cfg.gensym_types()) {
         if (name(type_decl).latest() == gensym_type) {
            name(type_decl).update(name(type_decl).latest() + "_" + gensym(16));
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
      auto path = Path(cfg.output()).replace_extension(".nim");
      require(write_file(path, output), "failed to write output file: ", path);
   } else {
      fatal("failed to execute visitor");
   }
}
} // namespace ensnare

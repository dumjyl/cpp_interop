#include "ensnare/private/main.hpp"

#include "ensnare/private/bit_utils.hpp"
#include "ensnare/private/builtins.hpp"
#include "ensnare/private/clang_utils.hpp"
#include "ensnare/private/config.hpp"
#include "ensnare/private/header_canonicalizer.hpp"
#include "ensnare/private/headers.hpp"
#include "ensnare/private/ir.hpp"
#include "ensnare/private/render.hpp"
#include "ensnare/private/runtime.hpp"
#include "ensnare/private/str_utils.hpp"
#include "ensnare/private/sym_generator.hpp"
#include "ensnare/private/utils.hpp"

#include "llvm/ADT/StringMap.h"

#include <random>
#include <tuple>

namespace ensnare {
/// Holds ensnare's state.
///
/// A `Context` must not outlive a `Config`
class Context {
   public:
   const clang::ASTContext& ast_ctx;

   private:
   Map<const clang::Decl*, Node<Type>>
       type_lookup; ///< For mapping bound declarations to exisiting types.
   public:
   Map<const clang::Decl*, std::tuple<Node<Type>, Node<TemplateParamDecl>>> templ_params;

   private:
   Vec<Node<TypeDecl>> _type_decls;         ///< Types to output.
   Vec<Node<RoutineDecl>> _routine_decls;   ///< Functions to output.
   Vec<Node<VariableDecl>> _variable_decls; ///< Variables to output.

   HeaderCanonicalizer header_canonicalizer;

   public:
   const Vec<Node<TypeDecl>>& type_decls() const { return _type_decls; }
   const Vec<Node<RoutineDecl>>& routine_decls() const { return _routine_decls; }
   const Vec<Node<VariableDecl>>& variable_decls() const { return _variable_decls; }

   private:
   Vec<const clang::NamedDecl*> decl_stack; ///< To give anonymous tags useful names, we track
                                            ///< the declaration they were referenced from.
   public:
   const Config& cfg;

   Context(const Config& cfg, const clang::ASTContext& ast_ctx)
      : cfg(cfg), ast_ctx(ast_ctx), header_canonicalizer(cfg, ast_ctx.getSourceManager()) {}

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

   void push(const clang::NamedDecl& decl) { decl_stack.push_back(&decl); }
   void pop_decl() { decl_stack.pop_back(); }

   private:
   const clang::Decl& canon_lookup_decl(const clang::Decl& decl) const {
      if (auto tag_decl = llvm::dyn_cast<clang::TagDecl>(&decl)) {
         return get_definition(*tag_decl);
      } else {
         return decl;
      }
   }

   public:
   /// Lookup the Node<Type> of a declaration.
   Opt<Node<Type>> lookup(const clang::Decl& decl) const {
      auto decl_type = type_lookup.find(&canon_lookup_decl(decl));
      if (decl_type == type_lookup.end()) {
         return {};
      } else {
         return decl_type->second;
      }
   }

   /// Record the Node<Type> of a declaration.
   void associate(const clang::Decl& decl, const Node<Type> type) {
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
   void add(const Node<TypeDecl> decl) { _type_decls.push_back(decl); }

   /// Add a routine declaration to be rendered.
   void add(const Node<RoutineDecl> decl) { _routine_decls.push_back(decl); }

   /// Add a variable declaration to be rendered.
   void add(const Node<VariableDecl> decl) { _variable_decls.push_back(decl); }

   /// Filters access to protected and private members.
   bool access_guard(const clang::Decl& decl) const {
      return decl.getAccess() == clang::AS_public || decl.getAccess() == clang::AS_none;
   }

   Opt<Str> maybe_header(const clang::NamedDecl& decl) {
      return header_canonicalizer[decl.getLocation()];
   }

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

Node<Type> map(Context& ctx, const clang::Type& decl);

/// Generic map for pointers to error on null.
template <typename T> Node<Type> map(Context& ctx, const T* entity) {
   if (entity) {
      return map(ctx, *entity);
   } else {
      fatal("tried to map a null entity");
   }
}

/// Maps to an atomic type of some kind. Many of these are obscure and unsupported.
Node<Type> map(Context& ctx, const clang::BuiltinType& entity) {
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
      case clang::BuiltinType::Kind::Char_S: fatal("unreachable: implicit signed char");
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
Node<Type> map(Context& ctx, const clang::QualType& entity) {
   // Too niche to care about for now.
   //  if (entity.isRestrictQualified() || entity.isVolatileQualified()) {
   //    fatal("volatile and restrict qualifiers unsupported");
   // }
   auto type = entity.getTypePtr();
   if (type) {
      auto result = map(ctx, *type);
      // FIXME: `isLocalConstQualified`: do we care about local vs non-local?
      if (entity.isConstQualified() && !ctx.cfg.ignore_const()) {
         return node<Type>(ConstType(result));
      } else {
         return result;
      }
   } else {
      fatal("QualType inner type was null");
   }
}

/// This maps certain kinds of declarations by first trying to look them up in the context.
/// If it is not in the context, we enforce wrapping it and return the mapped result.
Node<Type> map(Context& ctx, const clang::NamedDecl& decl) {
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

Tuple<Node<Type>, Node<TemplateParamDecl>> update_templ_param(Context& ctx,
                                                              const clang::NamedDecl& named_decl) {
   if (ctx.templ_params.count(&named_decl) == 0) {
      auto sym = node<Sym>(named_decl.getNameAsString());
      switch (named_decl.getKind()) {
         case clang::Decl::Kind::TemplateTypeParm: {
            auto& decl = llvm::cast<clang::TemplateTypeParmDecl>(named_decl);
            auto result = std::make_tuple(node<Type>(sym), node<TemplateParamDecl>(sym));
            ctx.templ_params[&decl] = result;
            return result;
         }
         case clang::Decl::Kind::NonTypeTemplateParm: {
            auto& decl = llvm::cast<clang::NonTypeTemplateParmDecl>(named_decl);
            auto result = std::make_tuple(
                node<Type>(sym),
                node<TemplateParamDecl>(sym, node<Type>(InstType(node<Sym>("static", true),
                                                                 {map(ctx, decl.getType())}))));
            ctx.templ_params[&decl] = result;
            return result;
         }
         default: write(render(named_decl)); fatal("unhandled template argument");
      }
   } else {
      return ctx.templ_params[&named_decl];
   }
}

Node<TemplateParamDecl> wrap_templ_param(Context& ctx, const clang::NamedDecl& decl) {
   auto [_, result] = update_templ_param(ctx, decl);
   return result;
}

Node<Type> map_templ_param(Context& ctx, const clang::NamedDecl& decl) {
   auto [result, _] = update_templ_param(ctx, decl);
   return result;
}

Node<Expr> make_expr(Context& ctx, const clang::Expr& expr) {
   switch (expr.getStmtClass()) {
      // this should be a non type template parameter.
      case clang::Stmt::DeclRefExprClass: {
         auto templ_param = wrap_templ_param(ctx, *llvm::cast<clang::DeclRefExpr>(expr).getDecl());
         return node<Expr>(ConstParamExpr(templ_param->name));
      }
      default: write(render(expr)); fatal("unhandled make_expr");
   }
}

Node<Type> map(Context& ctx, const clang::Type& type) {
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
         } else if (ty.isFunctionPointerType()) { // the ptr part is implicit for nim.
            return map(ctx, ty.getPointeeType());
         } else {
            return node<Type>(PtrType(map(ctx, ty.getPointeeType())));
         }
      }
      case clang::Type::TypeClass::LValueReference:
      case clang::Type::TypeClass::RValueReference:
         // FIXME: this may be wrong for function pointers.
         return node<Type>(
             RefType(map(ctx, llvm::cast<clang::ReferenceType>(type).getPointeeType())));
      case clang::Type::TypeClass::Vector:
         // The hope is that this is an aliased type as part of `typedef` / `using`
         // and the internals don't matter.
         return node<Type>(OpaqueType());
      case clang::Type::TypeClass::ConstantArray: {
         auto& ty = llvm::cast<clang::ConstantArrayType>(type);
         return node<Type>(ArrayType(node<Expr>(LitExpr(ty.getSize().getLimitedValue())),
                                     map(ctx, ty.getElementType())));
      }
      case clang::Type::TypeClass::DependentSizedArray: {
         auto& ty = llvm::cast<clang::DependentSizedArrayType>(type);
         return node<Type>(
             ArrayType(make_expr(ctx, *ty.getSizeExpr()), map(ctx, ty.getElementType())));
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

ParamDecl transfer(Context& ctx, const clang::ParmVarDecl& param) {
   return ParamDecl(param.getNameAsString(), map(ctx, param.getType()));
}

Vec<ParamDecl> transfer(Context& ctx, const clang::FunctionDecl& decl) {
   Vec<ParamDecl> result;
   for (auto param : decl.parameters()) {
      result.push_back(transfer(ctx, *param));
   }
   return result;
}

/// Map the return type if it has one.
Opt<Node<Type>> transfer_return_type(Context& ctx, const clang::FunctionDecl& decl) {
   auto result = decl.getReturnType();
   if (result->isVoidType()) {
      return {};
   } else {
      return map(ctx, result);
   }
}

void wrap_non_template(Context& ctx, const clang::FunctionDecl& decl) {
   ctx.add(node<RoutineDecl>(FunctionDecl(decl.getNameAsString(), qual_name(decl), ctx.header(decl),
                                          transfer(ctx, decl), transfer_return_type(ctx, decl))));
}

Node<Sym> type_sym(Node<Type> type) {
   if (is<Node<Sym>>(type)) {
      return as<Node<Sym>>(type);
   } else {
      fatal("Node<Type> is not a Node<Sym>");
   }
}

Vec<Node<TemplateParamDecl>> template_arguments(Context& ctx, const clang::TemplateDecl& decl) {
   Vec<Node<TemplateParamDecl>> result;
   for (auto argument : *decl.getTemplateParameters()) {
      result.push_back(wrap_templ_param(ctx, *argument));
   }
   return result;
}

void wrap_template(Context& ctx, const clang::FunctionDecl& decl) {
   ctx.add(node<RoutineDecl>(
       TemplateFunctionDecl(decl.getNameAsString(), qual_name(decl), ctx.header(decl),
                            template_arguments(ctx, *decl.getDescribedFunctionTemplate()),
                            transfer(ctx, decl), transfer_return_type(ctx, decl))));
}

void assert_non_template(const clang::FunctionDecl& decl) {
   switch (decl.getTemplatedKind()) {
      case clang::FunctionDecl::TK_NonTemplate: break;
      case clang::FunctionDecl::TK_FunctionTemplate:
      case clang::FunctionDecl::TK_MemberSpecialization:
      case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization:
         fatal("only simple function signatures are supported.");
   }
}

void wrap_function(Context& ctx, const clang::FunctionDecl& decl) {
   switch (decl.getTemplatedKind()) {
      case clang::FunctionDecl::TK_NonTemplate: wrap_non_template(ctx, decl); break;
      case clang::FunctionDecl::TK_FunctionTemplate: wrap_template(ctx, decl); break;
      case clang::FunctionDecl::TK_MemberSpecialization:
      case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization:
         fatal("only simple function signatures are supported.");
   }
}

void wrap_non_template(Context& ctx, const clang::CXXConstructorDecl& decl) {
   // We don't wrap anonymous constructors since they take a supposedly anonymous type as a
   // parameter.
   if (ctx.access_guard(decl) && has_name(*decl.getParent())) {
      ctx.add(node<RoutineDecl>(ConstructorDecl(qual_name(decl), ctx.header(decl),
                                                map(ctx, decl.getParent()), transfer(ctx, decl))));
   }
}

// fn wrap_template(Context& ctx, const clang::CXXConstructorDecl& decl) {
//    ctx.add(node<RoutineDecl>(TemplateConstructorDecl(
//        qual_name(decl), ctx.header(decl), template_arguments(ctx,
//        *decl.getDescribedFunctionTemplate()), map(ctx, decl.getParent()), transfer(ctx, decl))));
// }

void wrap_constructor(Context& ctx, const clang::CXXConstructorDecl& decl) {
   switch (decl.getTemplatedKind()) {
      case clang::FunctionDecl::TK_NonTemplate: wrap_non_template(ctx, decl); break;
      case clang::FunctionDecl::TK_FunctionTemplate: wrap_template(ctx, decl); break;
      case clang::FunctionDecl::TK_MemberSpecialization:
      case clang::FunctionDecl::TK_FunctionTemplateSpecialization:
      case clang::FunctionDecl::TK_DependentFunctionTemplateSpecialization:
         fatal("only simple function signatures are supported.");
   }
}

void wrap_non_template(Context& ctx, const clang::CXXMethodDecl& decl) {
   if (ctx.access_guard(decl)) {
      ctx.add(node<RoutineDecl>(MethodDecl(decl.getNameAsString(), qual_name(decl),
                                           ctx.header(decl), map(ctx, decl.getParent()),
                                           transfer(ctx, decl), transfer_return_type(ctx, decl))));
   }
}

void wrap(Context& ctx, const clang::CXXMethodDecl& decl) {
   assert_non_template(decl);
   wrap_non_template(ctx, decl);
}

void wrap_methods(Context& ctx, const clang::CXXRecordDecl::method_range methods) {
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
      } else {
         fatal("unreachable: wrap(CXXRecordDecl::method_range)");
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

Node<Sym> tag_name(Context& ctx, const clang::NamedDecl& decl) {
   return has_name(decl) ? node<Sym>(qual_nim_name(ctx, decl))
                         : node<Sym>("type_of(" + qual_nim_name(ctx, ctx.decl(1)) + ")");
}

Node<Sym> register_tag_name(Context& ctx, const clang::NamedDecl& name_decl,
                            const clang::TagDecl& def_decl) {
   auto result = tag_name(ctx, name_decl);
   auto type = node<Type>(result);
   ctx.associate(name_decl, type);
   if (!pointer_equality(name_decl, llvm::cast<clang::NamedDecl>(def_decl))) {
      ctx.associate(def_decl, type);
   }
   return result;
}

Str tag_import_name(Context& ctx, const clang::NamedDecl& decl) {
   return has_name(decl) ? decl.getQualifiedNameAsString()
                         : "decltype(" + ctx.decl(1).getQualifiedNameAsString() + ")";
}

void wrap_record_non_template(Context& ctx, const clang::NamedDecl& name_decl,
                              const clang::CXXRecordDecl& def_decl, bool force) {
   ctx.add(node<TypeDecl>(RecordTypeDecl(register_tag_name(ctx, name_decl, def_decl),
                                         tag_import_name(ctx, name_decl), ctx.header(name_decl),
                                         transfer(ctx, def_decl.fields()))));
   if (!force) {
      wrap_methods(ctx, def_decl.methods());
   }
}

Str template_record_import_name(Context& ctx, const clang::ClassTemplateDecl& templ_def_decl) {
   Str result = templ_def_decl.getQualifiedNameAsString() + "<";
   for (auto i = 0; i < template_arguments(ctx, templ_def_decl).size(); i += 1) {
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
   ctx.add(node<TypeDecl>(TemplateRecordTypeDecl(
       register_tag_name(ctx, name_decl, def_decl),
       template_record_import_name(ctx, templ_def_decl), ctx.header(name_decl),
       template_arguments(ctx, templ_def_decl), transfer(ctx, def_decl.fields()))));
   if (!force) {
      wrap_methods(ctx, def_decl.methods());
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
             EnumFieldDecl(field->getNameAsString(), field->getInitVal().getExtValue()));
      }
      ctx.add(node<TypeDecl>(EnumTypeDecl(register_tag_name(ctx, name_decl, def_decl),
                                          tag_import_name(ctx, name_decl), ctx.header(name_decl),
                                          fields)));
   }
}

void wrap_tag(Context& ctx, const clang::NamedDecl& name_decl, const clang::TagDecl& def_decl,
              bool force) {
   auto kind = def_decl.getKind();
   if (kind == clang::Decl::Kind::CXXRecord) {
      wrap_record(ctx, name_decl, llvm::cast<clang::CXXRecordDecl>(def_decl), force);
   } else if (kind == clang::Decl::Kind::Enum) {
      wrap_enum(ctx, name_decl, llvm::cast<clang::EnumDecl>(def_decl), force);
   } else {
      write(render(def_decl));
      fatal("unhandled: anon tag typedef");
   }
   // wrap_tag(ctx, register_tag_name(ctx, name_decl, def_decl), name_decl, def_decl, force);
}

void wrap_tag(Context& ctx, const clang::TagDecl& def_decl, bool force) {
   wrap_tag(ctx, def_decl, def_decl, force);
}

/// Check if `decl` is from <cstddef>.
Opt<Node<Type>> get_cstddef_item(Context& ctx, const clang::NamedDecl& decl) {
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
   auto name = node<Sym>(qual_nim_name(ctx, decl));
   ctx.associate(decl, node<Type>(name));
   auto type_decl = node<TypeDecl>(AliasTypeDecl(name, map(ctx, decl.getUnderlyingType())));
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
         default:
            write(render(named_decl));
            fatal("unhandled force_wrap: ", named_decl.getDeclKindName(), "Decl");
      }
      ctx.pop_decl();
   }
}

void wrap(Context& ctx, const clang::NamedDecl& named_decl) {
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
         case clang::Decl::Kind::ObjCIvar:
         case clang::Decl::Kind::ObjCAtDefsField: fatal("objc not supported");
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

/// The base wrapping routine that only filters out unnamed decls.
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
      case clang::Decl::Kind::TranslationUnit: {
         break; // discarded
      }
      case clang::Decl::Kind::firstNamed... clang::Decl::Kind::lastNamed: {
         wrap(ctx, llvm::cast<clang::NamedDecl>(decl));
         break;
      }
      default: write(render(decl)); fatal("unhandled wrapping: ", decl.getDeclKindName(), "Decl");
   }
}

Vec<Str> prefixed_search_paths() {
   Vec<Str> result;
   for (const auto& search_path : Header::search_paths()) {
      result.push_back("-isystem" + search_path);
   }
   return result;
}

/// Load a translation unit from user provided arguments with additional include path arguments.
std::unique_ptr<clang::ASTUnit> parse_translation_unit(const Config& cfg) {
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

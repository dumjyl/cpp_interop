#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

//
#include "ensnare/private/syn.hpp"

namespace ensnare {
constexpr bool debug = false;

/// Render an access specifier as a string. For debugging.
fn render(clang::AccessSpecifier as) -> Str {
   switch (as) {
      case clang::AS_none: return "AS_none";
      case clang::AS_public: return "AS_public";
      case clang::AS_private: return "AS_private";
      case clang::AS_protected: return "AS_protected";
   }
}

/// Render a declaration as a string. For debugging.
fn render(const clang::Decl& decl) -> Str {
   Str temp;
   llvm::raw_string_ostream stream(temp);
   decl.dump(stream);
   return stream.str();
}

/// Render a declaration as a string. For debugging.
fn render(const clang::Decl* decl) -> Str {
   if (decl) {
      return render(*decl);
   } else {
      return "null Decl";
   }
}

inline fn log(Str msg, const clang::NamedDecl& decl) {
   if constexpr (debug) {
      print(msg, " ", decl.getDeclKindName(), " ", decl.getQualifiedNameAsString());
   }
}

/// A version of TagDecl::getDefinition that fails gracefully.
template <typename T> fn get_definition(T& decl) -> const T& {
   auto result = decl.getDefinition();
   return result ? *result : decl;
}

/// A version of TagDecl::getDefinition that fails gracefully.
template <typename T> fn get_definition(T* decl) -> const T& {
   if (decl) {
      return get_definition(*decl);
   } else {
      fatal("unreachable: get_definition null");
   }
}

fn eq_ident(const clang::NamedDecl& a, const clang::NamedDecl& b) -> bool {
   // FIXME: find the right api.
   return a.getQualifiedNameAsString() == b.getQualifiedNameAsString();
}

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

/// Is this declaration declared without an identifier.
fn has_name(const clang::NamedDecl& decl) -> bool { return decl.getIdentifier(); }

/// Get the tag declaration that in defined inline with this typedef.
fn owns_tag(const clang::TypedefDecl& decl) -> OptRef<const clang::TagDecl> {
   // FIXME: how does a `typedef struct Foo tag`, where `struct Foo` is already declared.
   auto elaborated = llvm::dyn_cast<clang::ElaboratedType>(decl.getUnderlyingType());
   if (elaborated) {
      auto tag = elaborated->getOwnedTagDecl();
      if (tag) {
         return get_definition(*tag);
      }
   }
   return {};
}

template <typename T> using DeclVisitor = void (*)(T&, const clang::Decl&);

template <typename T>
inline fn visit(clang::ASTUnit& translation_unit, T& context, DeclVisitor<T> visitor) -> bool {
   class Visitor : public clang::RecursiveASTVisitor<Visitor> {
      T& context;
      DeclVisitor<T> visitor;
      pub bool result = false;

      pub Visitor(clang::ASTUnit& translation_unit, T& context, DeclVisitor<T> visitor)
         : context(context), visitor(visitor) {
         result =
             clang::RecursiveASTVisitor<Visitor>::TraverseAST(translation_unit.getASTContext());
      }

      pub fn VisitDecl(const clang::Decl* decl) -> bool {
         if (decl) {
            visitor(context, *decl);
         } else {
            fatal("unreachable: got a null visitor decl");
         }
         return true;
      }
   };
   Visitor instance(translation_unit, context, visitor);
   return instance.result;
}
} // namespace ensnare

#include "ensnare/private/undef_syn.hpp"

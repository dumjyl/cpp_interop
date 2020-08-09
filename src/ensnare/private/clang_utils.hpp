#pragma once

#include "ensnare/private/utils.hpp"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

namespace ensnare {
constexpr bool debug = false;

/// Render an access specifier as a string. For debugging.
Str render(clang::AccessSpecifier as);

/// Render a declaration as a string. For debugging.
Str render(const clang::Decl& decl);

/// Render a declaration as a string. For debugging.
Str render(const clang::Decl* decl);

/// Render an statement as a string. For debugging.
Str render(const clang::Stmt& stmt);

Str render(const clang::TemplateArgument& arg);

Str render(const clang::TemplateName& name);

Str render(const clang::Type& type);

Str render(clang::TemplateArgument::ArgKind kind);

inline void log(Str msg, const clang::NamedDecl& decl) {
   if constexpr (debug) {
      print(msg, " ", decl.getDeclKindName(), " ", decl.getQualifiedNameAsString());
   }
}

/// A version of TagDecl::getDefinition that fails gracefully.
template <typename T> const T& get_definition(T& decl) {
   auto result = decl.getDefinition();
   return result ? *result : decl;
}

/// A version of TagDecl::getDefinition that fails gracefully.
template <typename T> const T& get_definition(T* decl) {
   if (decl) {
      return get_definition(*decl);
   } else {
      fatal("unreachable: get_definition null");
   }
}

bool eq_ident(const clang::NamedDecl& a, const clang::NamedDecl& b);

/// Collect the relevant named declaration contexts for a declaration.
llvm::SmallVector<const clang::DeclContext*, 8> decl_contexts(const clang::NamedDecl& decl);

/// Is this declaration declared without an identifier.
bool has_name(const clang::NamedDecl& decl);

/// Get the tag declaration that in defined inline with this typedef.
OptRef<const clang::TagDecl> owns_tag(const clang::TypedefDecl& decl);

Str qual_name(const clang::NamedDecl& decl);

Str qual_name(const clang::NamedDecl* decl);

OptRef<const clang::TagDecl> inner_tag(const clang::TypedefDecl& decl);

template <typename T> using DeclVisitor = void (*)(T&, const clang::Decl&);

template <typename T>
inline bool visit(clang::ASTUnit& translation_unit, T& context, DeclVisitor<T> visitor) {
   class Visitor : public clang::RecursiveASTVisitor<Visitor> {
      private:
      T& context;
      DeclVisitor<T> visitor;

      public:
      bool result = false;

      Visitor(clang::ASTUnit& translation_unit, T& context, DeclVisitor<T> visitor)
         : context(context), visitor(visitor) {
         result =
             clang::RecursiveASTVisitor<Visitor>::TraverseAST(translation_unit.getASTContext());
      }

      bool VisitDecl(const clang::Decl* decl) {
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

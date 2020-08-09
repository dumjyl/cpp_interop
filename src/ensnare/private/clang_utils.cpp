#include "ensnare/private/clang_utils.hpp"

using namespace ensnare;

Str ensnare::render(clang::AccessSpecifier as) {
   switch (as) {
      case clang::AS_none: return "AS_none";
      case clang::AS_public: return "AS_public";
      case clang::AS_private: return "AS_private";
      case clang::AS_protected: return "AS_protected";
   }
}

Str ensnare::render(const clang::Decl& decl) {
   Str temp;
   llvm::raw_string_ostream stream(temp);
   decl.dump(stream);
   return stream.str();
}

Str ensnare::render(const clang::Decl* decl) {
   if (decl) {
      return render(*decl);
   } else {
      return "null Decl";
   }
}

Str ensnare::render(const clang::Stmt& stmt) {
   Str temp;
   llvm::raw_string_ostream stream(temp);
   stmt.dump(stream);
   return stream.str();
}

Str ensnare::render(const clang::TemplateArgument& arg) {
   Str temp;
   llvm::raw_string_ostream stream(temp);
   arg.dump(stream);
   return stream.str();
}

Str ensnare::render(const clang::TemplateName& name) {
   Str temp;
   llvm::raw_string_ostream stream(temp);
   name.dump(stream);
   return stream.str();
}

Str ensnare::render(const clang::Type& type) {
   Str temp;
   llvm::raw_string_ostream stream(temp);
   type.dump(stream);
   return stream.str();
}

Str ensnare::render(clang::TemplateArgument::ArgKind kind) {
   switch (kind) {
      case clang::TemplateArgument::Null: return "Null";
      case clang::TemplateArgument::Type: return "Type";
      case clang::TemplateArgument::Declaration: return "Declaration";
      case clang::TemplateArgument::NullPtr: return "NullPtr";
      case clang::TemplateArgument::Integral: return "Integral";
      case clang::TemplateArgument::Template: return "Template";
      case clang::TemplateArgument::TemplateExpansion: return "TemplateExpansion";
      case clang::TemplateArgument::Expression: return "Expression";
      case clang::TemplateArgument::Pack: return "Pack";
      default: return "unhandled: render(clang::TemplateArg::ArgKind)";
   }
}

bool ensnare::eq_ident(const clang::NamedDecl& a, const clang::NamedDecl& b) {
   // FIXME: find the right api.
   return a.getQualifiedNameAsString() == b.getQualifiedNameAsString();
}

llvm::SmallVector<const clang::DeclContext*, 8>
ensnare::decl_contexts(const clang::NamedDecl& decl) {
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

bool ensnare::has_name(const clang::NamedDecl& decl) { return decl.getIdentifier(); }

OptRef<const clang::TagDecl> ensnare::owns_tag(const clang::TypedefDecl& decl) {
   if (auto elaborated = llvm::dyn_cast<clang::ElaboratedType>(decl.getUnderlyingType())) {
      if (auto tag = elaborated->getOwnedTagDecl()) {
         return get_definition(*tag);
      }
   }
   return {};
}

Str ensnare::qual_name(const clang::NamedDecl& decl) { return decl.getQualifiedNameAsString(); }

Str ensnare::qual_name(const clang::NamedDecl* decl) { return qual_name(*decl); }

OptRef<const clang::TagDecl> ensnare::inner_tag(const clang::TypedefDecl& decl) {
   // FIXME: how does a `typedef struct Foo tag`, where `struct Foo` is already declared.
   if (auto elaborated = llvm::dyn_cast<clang::ElaboratedType>(decl.getUnderlyingType())) {
      if (auto tag_type = llvm::dyn_cast<clang::TagType>(elaborated->getNamedType().getTypePtr())) {
         return *tag_type->getDecl();
      }
   }
   return {};
}

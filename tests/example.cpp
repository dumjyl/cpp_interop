#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

class EchoNamesVisitor : public RecursiveASTVisitor<EchoNamesVisitor> {
 public:
  explicit EchoNamesVisitor(ASTContext *Context) : Context(Context) {}

  bool VisitNamedDecl(NamedDecl *decl) {
    llvm::outs() << decl->getQualifiedNameAsString() << '\n';
    return true;
  }

 private:
  ASTContext *Context;
};

class EchoNamesConsumer : public clang::ASTConsumer {
 public:
  explicit EchoNamesConsumer(ASTContext *Context) : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

 private:
  EchoNamesVisitor Visitor;
};

class EchoNamesAction : public clang::ASTFrontendAction {
 public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(
        new EchoNamesConsumer(&Compiler.getASTContext()));
  }
};

bool run(const char *src) {
  return clang::tooling::runToolOnCode(new EchoNamesAction, src);
}

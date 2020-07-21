/// \mainpage
/// This is internal documentation.
///
/// ## Architecture
///
/// ### Configuration
/// A simple CLI interface is used for now. We mostly forward arguments directly to libtooling.
///
/// Future work: Introspection is key to producing nice wrappers. For that we need a more procedural
/// api.
///
/// ### AST
/// Ensnare uses a symbolic representation. Both our input AST from clang and output AST is
/// symbolized (ours less rigorously). This is a mixed bag.
///
/// #### clang
/// The AST we recieve from clang is fully typed. This means all PP directives have already been
/// expanded. What we lose in preprocessor information, we gain in standard conformant semantic
/// information.
///
/// #### ensnare
/// The symbolic representation allows name mangling after a symbol has been defined.
/// Care must be taken to not produce multiple symbols for the same declaration.
///
/// ### Visiting
/// Our visitor is responsible for instiating a clang::ASTUnit and a Context.
/// We bind or discard on _every_ clang::Decl wihnin our translation unit.
///
/// ### Binding
/// The core binding code consists of some basic operations.
///
/// #### wrapping
/// Wrapping possibly adds c++ declarations in the form of clang::Decl to our Context.
/// We can filter what we wrap with privacy info, source location, symbolic location in the form of
/// namespaces, typespaces (namespaces, typespaces, etc.). Some of that is aspirational/buggy.
///
/// When we choose to wrap something
///
/// #### mapping
/// While the purpose of wrapping is intuitive
/// Mapping must produce a Node<Type> from some entity.
///
/// ## Future work
/// - A target system for cross compilation and arch specific features/builtins.
///   Cross compilation is already exposed with `--disable-include-search-paths` and clang's
///   `--target`.
/// - clang offers some PP information that should be exposed as constants and templates.

#pragma once

/// %Main project namespace.
namespace ensnare {
void run(int argc, const char* argv[]);
}

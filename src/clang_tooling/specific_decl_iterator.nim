type
  specific_decl_iterator*[T] {.import_cpp: "clang::DeclContext::specific_decl_iterator<'0>",
                               header: "clang/AST/DeclBase.h".} = object

proc `!=`*[T](a, b: specific_decl_iterator[T]): bool
  {.import_cpp: "(# != #)", header: "clang/AST/DeclBase.h".}

proc succ*[T](self: specific_decl_iterator[T]): specific_decl_iterator[T]
  {.import_cpp: "#.operator++()", header: "clang/AST/DeclBase.h".}

proc deref*[T](self: specific_decl_iterator[T]): ptr T
  {.import_cpp: "#.operator*()", header: "clang/AST/DeclBase.h".}

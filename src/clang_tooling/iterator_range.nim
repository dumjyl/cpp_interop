type
  iterator_range*[T] {.import_cpp: "llvm::iterator_range<'0>",
                       header: "llvm/ADT/iterator_range.h".} = object

proc begin*[T](self: iterator_range[T]): T
  {.import_cpp: "#.begin(@)", header: "llvm/ADT/iterator_range.h".}

proc `end`*[T](self: iterator_range[T]): T
  {.import_cpp: "#.end(@)", header: "llvm/ADT/iterator_range.h".}

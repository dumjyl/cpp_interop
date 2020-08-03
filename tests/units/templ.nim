import ensnare/runtime
export runtime

proc foo*[T](a: T, b: T): T
   {.import_cpp: "foo(@)", header: "templ.hpp".}

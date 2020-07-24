import ensnare/runtime
export runtime

type
   outer* = inner
   inner* {.import_cpp: "inner", header: "redecls.hpp".} = object
      next: ptr outer

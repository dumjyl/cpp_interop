import ensnare/runtime
export runtime

type
   lexbor_mem_chunk_t* = lexbor_mem_chunk
   lexbor_mem_chunk* {.import_cpp: "lexbor_mem_chunk", header: "redecls2.hpp".} = object
      next: ptr lexbor_mem_chunk_t
   lexbor_mem* {.import_cpp: "lexbor_mem", header: "redecls2.hpp".} = object
      chunk: ptr lexbor_mem_chunk_t
   lexbor_mem_t* = lexbor_mem
   foo_t* = foo
   foo* {.import_cpp: "foo", header: "redecls.hpp".} = object
      base: ptr foo_t

proc `{}`*(�: type[lexbor_mem_chunk]): lexbor_mem_chunk
   {.import_cpp: "lexbor_mem_chunk::lexbor_mem_chunk(@)", header: "redecls2.hpp".}
proc `{}`*(�: type[lexbor_mem_chunk], �1: var lexbor_mem_chunk): lexbor_mem_chunk
   {.import_cpp: "lexbor_mem_chunk::lexbor_mem_chunk(@)", header: "redecls2.hpp".}
proc `{}`*(�: type[lexbor_mem_chunk], �1: var lexbor_mem_chunk): lexbor_mem_chunk
   {.import_cpp: "lexbor_mem_chunk::lexbor_mem_chunk(@)", header: "redecls2.hpp".}

var
   type_map* {.import_cpp: "type_map", header: "redecls.hpp".}: lexbor_mem_chunk_t

#include "redecls2.hpp"

typedef struct foo foo_t;

struct foo {
   foo_t* base;
};

lexbor_mem_chunk_t type_map;

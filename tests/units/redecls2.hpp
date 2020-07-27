typedef struct lexbor_mem_chunk lexbor_mem_chunk_t;
typedef struct lexbor_mem lexbor_mem_t;

struct lexbor_mem_chunk {
   lexbor_mem_chunk_t* next;
};

struct lexbor_mem {
   lexbor_mem_chunk_t* chunk;
};

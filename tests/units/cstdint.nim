import ensnare/runtime

# routine section
proc test(a: cpp_size, b: cpp_size)
   {.import_cpp: "test".}

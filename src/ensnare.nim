import ensnare/runtime, ensnare/private/[app_utils, build], std/os

build.flags()

const hpp = CppSrc{"ensnare/private/ensnare.hpp"}

proc `ensnare-run`(argc: cpp_int, argv: cpp_unsized_array[cpp_char_ptr]) {.cpp_load: hpp.}

proc main(argc: cpp_int, argv: cpp_unsized_array[cpp_char_ptr]): cpp_int {.export_c.} =
   run(argc, argv)
   result = 0

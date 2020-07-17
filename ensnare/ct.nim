import rt, private/[app_utils, build]

build.flags()

const ct_hpp = CppSrc{"ct.hpp"}

proc `ensnare-ct-run`(argc: cpp_int, argv: cpp_unsized_array[cpp_char_ptr])
   {.cpp_load: ct_hpp.}


proc main(argc: cpp_int, argv: cpp_unsized_array[cpp_char_ptr]): cpp_int {.export_c.} =
   run(argc, argv)
   result = 0

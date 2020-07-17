import rt, private/[app_utils, build]

build.flags()

const ct_cpp = CppSrc{"ct.hpp"}

proc `ensnare-ct-run`(argc: cpp_int, argv: cpp_unsized_array[cpp_char_ptr]): bool
   {.cpp_load: ct_cpp.}

proc main(argc: cpp_int, argv: cpp_unsized_array[cpp_char_ptr]): cpp_int {.export_c.} =
   if run(argc, argv):
      echo "ensnare binding succeeded"
   else:
      echo "ensnare binding failed"

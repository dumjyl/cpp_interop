import ensnare/runtime, ensnare/private/[app_utils, build], std/os

cpp_compile_src("ensnare"/"private"/"os_utils.cpp")
cpp_compile_src("ensnare"/"private"/"str_utils.cpp")
cpp_compile_src("ensnare"/"private"/"headers.cpp")
cpp_compile_src("ensnare"/"private"/"ir.cpp")
build.flags()

const hpp = CppSrc{"ensnare/private/ensnare.hpp"}

proc `ensnare-run`(argc: CppInt, argv: CppUnsizedArray[CppCharPtr]) {.cpp_load: hpp.}

proc main(argc: CppInt, argv: CppUnsizedArray[CppCharPtr]): CppInt {.export_c.} =
   run(argc, argv)
   result = 0

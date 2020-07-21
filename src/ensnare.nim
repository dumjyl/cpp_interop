import ensnare/runtime, ensnare/private/[app_utils, build], std/os

cpp_compile_src("ensnare"/"private"/"os_utils.cpp")
cpp_compile_src("ensnare"/"private"/"str_utils.cpp")
cpp_compile_src("ensnare"/"private"/"headers.cpp")
cpp_compile_src("ensnare"/"private"/"ir.cpp")
cpp_compile_src("ensnare"/"private"/"config.cpp")
cpp_compile_src("ensnare"/"private"/"rendering.cpp")
cpp_compile_src("ensnare"/"private"/"main.cpp")
build.flags()

proc run(argc: CppInt, argv: CppUnsizedArray[CppCharPtr])
   {.import_cpp: "ensnare::run(@)", header: "ensnare/private/main.hpp".}

proc main(argc: CppInt, argv: CppUnsizedArray[CppCharPtr]): CppInt {.export_c.} =
   run(argc, argv)
   result = 0

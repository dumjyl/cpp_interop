import ensnare/runtime, ensnare/private/build
from std/os import `/`

const deps = get_project_path().parent_dir/"deps"
cpp_include_dir(deps/"sugar"/"include")
cpp_compile_src_dir(deps/"sugar"/"src")
cpp_compile_src_dir("ensnare"/"private")
build.flags()

proc run(argc: CppInt, argv: CppUnsizedArray[CppCharPtr])
   {.import_cpp: "ensnare::run(@)", header: "ensnare/private/main.hpp".}

proc main(argc: CppInt, argv: CppUnsizedArray[CppCharPtr]): CppInt {.export_c.} =
   run(argc, argv)
   result = 0

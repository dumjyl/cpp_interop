# This only compiling/linking flags, not the nim api.

import
  clang_tooling,
  os

{.compile: "example.cpp".}

proc run(src: c_string): bool
  {.import_cpp: "run(@)", header: "example.hpp".}

let src = read_file(param_str(1))

if not run(src):
  quit("clang tooling failed", 1)

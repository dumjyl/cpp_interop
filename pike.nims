import os
let include_dir = gorge("llvm-config --includedir")
arg "-I" & include_dir
input include_dir/"clang/AST/DeclarationName.h"
output "src_v2"/"declaration_name.nim"
sym ns"clang"/"DeclarationName"/"NameKind"

import
   pkg/std_ext/[os,
                options,
                str_utils,
                seq_utils]

when not defined(linux):
   {.error: "TODO: other platform supprt".}

let llvm_dir = block:
   var (output, code) = os.exec("llvm-config", ["--prefix"])
   assert(code == 0)
   output.strip_line_end()
   output

import ../src/"clang_tooling.nims"

let dir = "clang-frontend"
let lib_dir = dir/"lib"
let include_dir = dir/"include"
let bin_dir = dir/"bin"

create_dir(dir)
create_dir(lib_dir)
create_dir(include_dir)
create_dir(bin_dir)

proc extract_lib_name(file: string): string =
   let name = file.extract_filename()
   do_assert(file[0 ..< 3] == "lib" and file[^2 .. ^1] == ".a")
   result = file[4 ..< ^3]

for lib in walk_files(llvm_dir/"lib"):

   let libname = "lib" & lib & ".a"
   copy_file(llvm_dir/"lib"/lib_name, lib_dir/lib_name)

copy_dir(llvm_dir/"include"/"clang", include_dir/"clang")

proc copy_file_with_permissions(src, dest: string) =
   copy_file(src, dest)
   dest.set_file_permissions(src.get_file_permissions())

copy_file_with_permissions(llvm_dir/"bin"/"llvm-config",
                           bin_dir/"clang-frontend-config")

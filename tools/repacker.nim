import
   pkg/std_ext,
   pkg/std_ext/[os,
                str_utils],
   build

proc touch(file: string) =
   file.write_file("")

iterator list_files(dir: string): string =
   for entry in walk_dir(dir):
      if entry.kind == pc_file:
         yield entry.path

proc remove_prefix(s: string, prefix: string): string =
   result = s
   result.remove_prefix(prefix)

let from_dist = param_str(1)
let version = param_str(2)
let suffix = param_str(3)
let platform = param_str(4)

let dist = "clang-tooling-" & version & "-" & suffix
remove_dir(dist)
create_dir(dist)

let bins = dist/"bin"
create_dir(bins)
case platform:
of "win":
   copy_file_with_permissions(from_dist/"bin"/"llvm-config.exe",
                              dist/"bin"/"clang-tooling-config.exe", false)
of "linux":
   copy_file_with_permissions(from_dist/"bin"/"llvm-config",
                              dist/"bin"/"clang-tooling-config", false)
else:
   fatal "Unsupported platform: ", platform

let libs = dist/"lib"
create_dir(libs)
for file in list_files(from_dist/"lib"):
   var name = file.split_file().name.remove_prefix("lib")
   let out_file = dist & file.substr(from_dist.len)
   #echo file, " -> ", out_file
   if name in llvm_libs or name in clang_libs: copy_file(file, out_file)
   elif name.starts_with("clang"): discard
   else: touch(out_file)
let includes = dist/"include"
create_dir(includes)
copy_dir(from_dist/"include"/"clang", dist/"include"/"clang")
copy_dir(from_dist/"include"/"clang-c", dist/"include"/"clang-c")
copy_dir(from_dist/"include"/"llvm", dist/"include"/"llvm")
copy_dir(from_dist/"include"/"llvm-c", dist/"include"/"llvm-c")

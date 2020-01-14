import
   pkg/std_ext,
   pkg/std_ext/[build,
                options,
                os]
from macros import error
export link

const
   clang_libs* = [
      "clangTooling",
      "clangDriver",
      "clangFrontend",
      "clangSerialization",
      "clangParse",
      "clangSema",
      "clangEdit",
      "clangAnalysis",
      "clangAST",
      "clangLex",
      "clangBasic"]
   llvm_libs* = [
      "LLVMMCParser",
      "LLVMMC",
      "LLVMDebugInfoCodeView",
      "LLVMDebugInfoMSF",
      "LLVMBitstreamReader",
      "LLVMProfileData",
      "LLVMCore",
      "LLVMRemarks",
      "LLVMOption",
      "LLVMBinaryFormat",
      "LLVMSupport",
      "LLVMDemangle"]

#proc find_exe_opt(exe: string): Opt[string] =
#   let exe = find_exe(exe)
#   if exe.len == 0:
#      result = none(string)
#   else:
#      result = some(exe)

proc config(arg: string): string =
   let config_exe = anon:
      # XXX: can't use find_exe at compile time.
      #if find_exe("clang-frontend-config") as some(clang_frontend_cfg):
      #   result = clang_frontend_cfg
      #elif find_exe("llvm-config") as some(llvm_cfg):
      #   result = llvm_cfg
      #else:
      #   error("Cannot find suitable configuration executable")
      "llvm-config"
   result = static_exec(config_exe & " --" & arg)

template flags* =
   {.pass_c: config"cxxflags".}
   {.pass_c: "-fexceptions".}
   link(clang_libs & llvm_libs)
   {.pass_l: config"ldflags".}
   {.pass_l: config"system-libs".}

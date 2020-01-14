import
   pkg/std_ext,
   pkg/std_ext/options
from macros import error

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

when defined(nim_script):
   proc find_exe_opt(exe: string): Opt[string] =
      let exe = find_exe(exe)
      if exe.len == 0:
         result = none(string)
      else:
         result = some(exe)

   proc config(arg: string): string =
      let config_exe = anon:
         if find_exe_opt("clang-frontend-config") as some(clang_frontend_cfg):
            result = clang_frontend_cfg
         elif find_exe_opt("llvm-config") as some(llvm_cfg):
            result = llvm_cfg
         else:
            error("Cannot find suitable configuration executable")
      result = static_exec(config_exe & " --" & arg)

   switch("pass_c", config"cxxflags")
   switch("pass_c", "-fexceptions")
   for lib in clang_libs & llvm_libs:
      switch("pass_l", "-l" & lib)
   switch("pass_l", config"ldflags")
   switch("pass_l", config"system-libs")

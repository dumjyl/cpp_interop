import
   pkg/std_ext,
   pkg/std_ext/[build,
                str_utils,
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

proc valid_config(exe: string): bool =
   var (output, code) = gorge_ex(exe & " --version")
   output.strip_line_end()
   result = code == 0 and output.starts_with("9.0.")

proc config(arg: string): string =
   let exe = anon:
      if valid_config("clang-tooling-config"):
         result = "clang-tooling-config"
      elif valid_config("llvm-config"):
         result = "llvm-config"
      else:
         error("failed to find valid llvm-config")
   let (output, code) = gorge_ex(exe & " --" & arg)
   if code != 0 or output.len == 0:
      if output.len == 0:
         error(exe & " failed with code '" & $code & "' and no output")
      else:
         error(exe & " failed with code '" & $code & "' and output:\n" &
               output)
   else:
      result = output
      result.strip_line_end()

template flags* =
   {.pass_c: config"cxxflags".}
   {.pass_c: "-fexceptions".}
   link(clang_libs & llvm_libs)
   when defined(windows):
      link("shlwapi")
      link("version")
   {.pass_l: config"ldflags".}
   {.pass_l: config"system-libs".}

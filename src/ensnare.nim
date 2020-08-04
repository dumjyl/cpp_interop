import ensnare/runtime

from std/os import `/`
from std/macros import error
from std/strutils import strip_line_end, starts_with

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
      "LLVMFrontendOpenMP",
      "LLVMProfileData",
      "LLVMCore",
      "LLVMRemarks",
      "LLVMOption",
      "LLVMBinaryFormat",
      "LLVMSupport",
      "LLVMDemangle"]

proc valid_config(exe: string): bool =
   var (output, code) = gorge_ex(exe & " --version")
   output.strip_line_end
   result = code == 0 and output.starts_with("10.0.")

proc find_config(arg: string): string =
   if valid_config("clang-tooling-config"):
      result = "clang-tooling-config"
   elif valid_config("llvm-config"):
      result = "llvm-config"
   else:
      error("failed to find valid llvm-config")

proc config(arg: string): string =
   var exe = find_config(arg)
   let (output, code) = gorge_ex(exe & " --" & arg)
   if code != 0 or output.len == 0:
      if output.len == 0:
         error(exe & " failed with code '" & $code & "' and no output")
      else:
         error(exe & " failed with code '" & $code & "' and output:\n" & output)
   else:
      result = output
      result.strip_line_end

const deps = get_project_path().parent_dir/"deps"
cpp_include_dir(deps/"sugar"/"include")
cpp_compile_src_dir(deps/"sugar"/"src")
cpp_compile_src_dir("ensnare"/"private")

cpp_include_dir(config("includedir"))
cpp_forward_compiler("-fno-rtti")
cpp_forward_compiler("-D_GNU_SOURCE")
cpp_forward_compiler("-D__STDC_CONSTANT_MACROS")
cpp_forward_compiler("-D__STDC_FORMAT_MACROS")
cpp_forward_compiler("-D__STDC_LIMIT_MACROS")
cpp_link_lib(@clang_libs & @llvm_libs)
cpp_link_lib("omptarget")
cpp_link_lib("stdc++fs")
when defined(windows):
   cpp_link_lib("shlwapi")
   cpp_link_lib("version")
cpp_forward_linker(config("ldflags"))
cpp_forward_linker(config("system-libs"))

proc run(argc: CppInt, argv: CppUnsizedArray[CppCharPtr])
   {.import_cpp: "ensnare::run(@)", header: "ensnare/private/main.hpp".}

when is_main_module:
   proc main(argc: CppInt, argv: CppUnsizedArray[CppCharPtr]): CppInt {.export_c.} =
      run(argc, argv)
      result = 0

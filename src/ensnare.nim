import ensnare/runtime

from std/os import `/`
from std/macros import error
from std/strutils import strip_line_end, starts_with

const libs* = [
   "clangTooling",
   "clangFrontend",
   "clangDriver",
   "clangSerialization",
   "clangParse",
   "clangSema",
   "clangEdit",
   "clangAnalysis",
   "clangAST",
   "clangLex",
   "clangBasic",
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

proc find_config(): string =
   if valid_config("clang-tooling-config"):
      result = "clang-tooling-config"
   elif valid_config("llvm-config"):
      result = "llvm-config"
   else:
      error("failed to find valid llvm-config")

proc config(exe: string, arg: string): string =
   let (output, code) = gorge_ex(exe & " --" & arg)
   if code != 0 or output.len == 0:
      if output.len == 0:
         error(exe & " failed with code '" & $code & "' and no output")
      else:
         error(exe & " failed with code '" & $code & "' and output:\n" & output)
   else:
      result = output
      result.strip_line_end

const
   deps = get_project_path().parent_dir/"deps"
   exe = find_config()

cpp_include_dir(deps/"sugar"/"include")
cpp_compile_src_dir(deps/"sugar"/"src")
cpp_compile_src_dir("ensnare"/"private")
cpp_forward_compiler(exe.config("cxxflags") & " -std=c++17 -fexceptions")
cpp_forward_linker(exe.config("ldflags"))
cpp_link_lib(libs)
cpp_link_lib("stdc++fs")
when defined(windows):
   cpp_link_lib("shlwapi")
   cpp_link_lib("version")
when defined(docker_alpine):
   cpp_link_lib("z")
   cpp_forward_linker("-static")
else:
   cpp_forward_linker(exe.config("system-libs"))

proc run(argc: CppInt, argv: CppUnsizedArray[CppCharPtr])
   {.import_cpp: "ensnare::run(@)", header: "ensnare/private/main.hpp".}

when is_main_module:
   proc main(argc: CppInt, argv: CppUnsizedArray[CppCharPtr]): CppInt {.export_c.} =
      run(argc, argv)
      result = 0

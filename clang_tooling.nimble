version = "0.5.0"
author= "Jasper Jenkins"
description = "bindings and stuff for the clang compiler"
license = "MIT"
backend = "cpp"
src_dir = "src"

requires "https://github.com/dumjyl/std-ext >= 1.5.1"

import
   os,
   strutils

task test, "Run tests":
   const expected_output = """
an_int
foo
foo::NiceClass
foo::NiceClass::with_a_field
foo::NiceClass::and_a_method"""
   let src = "tests"/"example.nim"
   let input = "tests"/"input.cpp"
   var (output, code) = gorge_ex("nim cpp -r --list_cmd " & src & " " & input)
   output.strip_line_end()
   if code != 0 or not output.ends_with(expected_output):
      raise newException(Defect, "tests failed with code '" & $code &
                                 "' and output:\n" & output)
   else:
      echo "Tests successful"

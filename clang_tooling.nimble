version = "0.1.0"
author= "Jasper Jenkins"
description = "bindings and stuff for the clang compiler"
license = "MIT"
backend = "cpp"
src_dir = "src"

requires "https://github.com/dumjyl/std-ext >= 1.5.0"

task test, "Run tests":
   const expected_output = """
an_int
foo
foo::NiceClass
foo::NiceClass::with_a_field
foo::NiceClass::and_a_method"""
   let (output, code) = gorge_ex "nim cpp -r tests/example.nim tests/input.cpp"
   if code != 0 or not output.ends_with(expected_output):
      quit("tests failed with code '" & $code & "' and output:\n" & output, 1)
   else:
      echo "Tests successful"

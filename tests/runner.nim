import ensnare/private/[os_utils, app_utils], std/os
from strutils import indent

const tests = ["typedefs", "abc", "redecls"]

proc nim_gen_file(name: string): string = "tests"/"units"/"gen"/name.change_file_ext(".nim")

proc nim_test_file(name: string): string = "tests"/"units"/name.change_file_ext(".nim")

proc hpp_test_file(name: string): string = "tests"/"units"/name.change_file_ext(".hpp")

proc invoke(name: string): (string, int) =
   result = exec("bin/ensnare", [nim_gen_file(name), "-I" & "tests"/"units",
                                 name.change_file_ext(".hpp")])

proc units =
   for test in tests:
      let (output, code) = invoke(test)
      if code == 0:
         if output.len != 0:
            echo "Ensnare Output:\n"
            echo indent(output, 3)
         let (diff_output, diff_code) = exec("diff", ["--color=always", "-u",
                                                      nim_test_file(test),
                                                      nim_gen_file(test)])
         if diff_code == 0:
            echo "Test Success: ", test
         else:
            echo "Test Failure: ", test
            echo "Code: ", code
            echo "Diff:\n", diff_output
            quit 1
      else:
         echo "Test Failure: ", test
         echo "Code: ", code
         echo "Output:\n", indent(output, 3)
         quit 1

main:
   units()

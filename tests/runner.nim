import ensnare/private/[os_utils, app_utils], std/os


proc nim_gen_file(name: string): string = "tests"/"units"/"gen"/name.change_file_ext(".nim")

proc nim_test_file(name: string): string = "tests"/"units"/name.change_file_ext(".nim")

proc hpp_test_file(name: string): string = "tests"/"units"/name.change_file_ext(".hpp")

proc invoke(name: string): (string, int) =
   result = exec("bin/ensnare", [nim_gen_file(name), hpp_test_file(name)])

proc units =
   for test in ["abc", "cstdint"]:
      let (output, code) = invoke(test)
      if code == 0:
         if read_file(nim_gen_file(test)) == read_file(hpp_test_file(test)):
            echo "Success: ", test
         else:
            let (diff_output, diff_code) = exec("diff", ["--color=always", "-u",
                                                         nim_test_file(test),
                                                         nim_gen_file(test)])
            echo "Failure: ", test
            echo "Code: ", code
            echo "Diff:\n", diff_output
      else:
         echo "Failure: ", test
         echo "Code: ", code
         echo "Output:\n", output

main:
   units()

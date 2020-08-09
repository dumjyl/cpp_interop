import ensnare/private/[os_utils, app_utils], std/os
from std/strutils import indent, split, starts_with, strip

type
   TestSection = object
      directives: seq[string]
      src: string
   Test = object
      bindings: string
      program: string

proc consume(i: var int, lines: openarray[string]): TestSection =
   while lines[i].starts_with("#%"):
      result.directives.add(lines[i][2 .. ^1].strip)
      inc i
   while i != lines.high and not lines[i].starts_with("#%"):
      result.src.add(lines[i] & '\n')
      inc i

proc `{}`*(Self: type[Test], path: string): Self =
   let lines = read_file(path).split('\n')
   if lines.len == 0:
      fatal("got empty test: ", path)
   var sections = default seq[TestSection]
   var i = 0
   while i != lines.high:
      sections.add(consume(i, lines))
   for section in sections:
      if section.directives.len == 0 and result.bindings.len == 0:
         result.bindings = section.src.strip(leading=false, trailing=true) & '\n'
      elif section.directives.len == 1 and section.directives[0] == "run":
         result.program = section.src
      else:
         fatal("failed to parse directives: ", $section.directives)

const tests = ["typedefs", "abc", "redecls", "templ"]
const units = "tests"/"units"

proc nim_gen_file(name: string): string = units/"gen"/name.change_file_ext(".nim")

proc nim_test_file(name: string): string = units/name.change_file_ext(".nim")

proc run_tests =
   for test in tests:
      let test_path = units/test.change_file_ext(".nim")
      let test_spec = Test{test_path}
      let (output, code) = exec("bin/ensnare", ["-include-dir=" & units, nim_gen_file(test),
                                                test.change_file_ext(".hpp")])

      if code == 0:
         if output.len != 0:
            echo "Ensnare Output:\n"
            echo indent(output, 3)
         let diff_file = nim_gen_file(test & "-diff")
         write_file(diff_file, test_spec.bindings)
         let (diff_output, diff_code) = exec("diff", ["--color=always", "-u", diff_file,
                                                      nim_gen_file(test)])
         if diff_code == 0:
            if test_spec.program.len != 0:
               # FIXME: this shuold be its own file and import the bindings instead.
               let (output, code) = exec("nim", ["cpp", "-r", test_path])
               if code == 0:
                  echo "Test Success: ", test
               else:
                  echo "Test Failure: ", test
                  echo "Code: ", code
                  echo "Output:\n", indent(output, 3)
                  quit 1
            else:
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
   run_tests()

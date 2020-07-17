import std/strutils, std/os, ../ensnare/private/os_utils

proc get_files(dir: string): seq[string] =
   for entry in walk_dir(dir):
      case entry.kind:
      of pc_dir: result.add(get_files(entry.path))
      of pc_file:
         if entry.path.split_file.ext in [".hpp", ".cpp"]:
            result.add(entry.path)
      else: discard

proc process(str: string): string =
   for line in str.split('\n'):
      if line.contains("fn") and line.contains("->"):
         let i = line.find("->")
         result.add(line[0 ..< i])
         result.add(" -> ")
         result.add(line[i + 2 .. ^1] & "\n")
      else:
         result.add(line & "\n")

proc main =
   for file in get_files(param_str(1)):
      let (output, code) = exec("clang-format", [file])
      assert(code == 0, "failed for: " & file)
      write_file(file, output.process[0 .. ^1])

when is_main_module: main()

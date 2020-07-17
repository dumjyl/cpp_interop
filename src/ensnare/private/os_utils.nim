import osproc, streams

proc exec*(
      command: string,
      args: openarray[string] = [],
      options = {po_stderr_to_stdout, po_use_path},
      working_dir = ""
      ): tuple[output: string, code: int] =
   ## Return the output and code of a command.
   let p = start_process(command, args=args, options=options, working_dir=working_dir)
   let output = output_stream(p)
   var line = new_string_of_cap(120)
   while true:
      if read_line(output, line):
         result.output.add(line)
         result.output.add("\n")
      else:
         result.code = peek_exit_code(p)
         if result.code != -1:
            break
   close(p)

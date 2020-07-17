from std/strutils import join

type ExitRequest* = object of CatchableError
   code: int

template main*(stmts) =
   when is_main_module:
      proc main =
         try:
            stmts
         except ExitRequest as er:
            quit(er.msg, er.code)
      main()

template request_exit*(code: int, msg: string) =
   let er = new_Exception(ExitRequest, msg)
   er.code = code
   raise er

template fatal*(msg: varargs[string, `$`]) =
   request_exit(1, "fatal-error: " & join(msg))

type
  Twine* {.import_cpp: "llvm::Twine",
           header: "llvm/ADT/Twine.h".} = object

proc init*(Self: type[Twine], str: c_string): Twine
  {.import_cpp: "'0(@)".}

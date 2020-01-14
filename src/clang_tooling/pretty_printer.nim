import
  ./lang_options

const
  ppH = "clang/AST/PrettyPrinter.h"

type
  PrintingPolicy*{.importcpp: "clang::PrintingPolicy", header: ppH.} = object

proc newPrintingPolicy*(lo: LangOptions): PrintingPolicy
  {.importcpp: "clang::PrintingPolicy(@)", header: ppH.}

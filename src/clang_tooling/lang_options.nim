const
  loH = "clang/Basic/LangOptions.h"

type
  LangOptions*{.importcpp: "clang::LangOptions", header: loH.} = object

proc newLangOptions*: LangOptions
  {.importcpp: "clang::LangOptions(@)", constructor.}

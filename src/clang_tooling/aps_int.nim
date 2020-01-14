const
  H = "llvm/ADT/APSInt.h"

type
  APSInt*{.importcpp: "llvm::APSInt", header: H.} = object

proc getExtValue*(self: APSInt): int64
  {.importcpp: "getExtValue", header: H.}

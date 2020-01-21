const
  H = "llvm/ADT/APSInt.h"

type
  APSInt*{.importcpp: "llvm::APSInt", header: H.} = object

proc get_ext_value*(self: APSInt): int64
  {.importcpp: "#.getExtValue(@)", header: H.}

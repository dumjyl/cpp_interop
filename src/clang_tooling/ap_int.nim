const
  apIntH = "llvm/ADT/APInt.h"

type
  APInt*{.import_cpp: "llvm::APInt", header: apIntH.} = object

proc get_limited_value*(this: APInt, limit = high(uint64)): uint64
  {.import_cpp: "#.getLimitedValue(@)", header: apIntH.}

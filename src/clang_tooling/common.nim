const
   cast_support = "llvm/Support/Casting.h"

proc is_a*[T0, T1](val: T0, _: typedesc[T1]): bool
   {.import_cpp: "llvm::isa<'2>(@)", header: cast_support.}

proc cast_to*[T0, T1](val: ptr T0, _: typedesc[T1]): ptr T1
   {.import_cpp: "llvm::cast<'2>(@)", header: cast_support.}
